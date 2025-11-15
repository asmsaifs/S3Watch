// QMI8658-based step counting and activity classification with raise-to-wake

#include "sensors.h"
#include "bsp/esp32_s3_touch_amoled_2_06.h"
#include "display_manager.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "qmi8658.h"
#include <math.h>
#include <time.h>

#define IMU_IRQ_GPIO GPIO_NUM_21
#define IMU_ADDR_HIGH QMI8658_ADDRESS_HIGH
#define IMU_ADDR_LOW QMI8658_ADDRESS_LOW

// Raise-to-wake sensitivity (tune to taste)
#define RAISE_DP_THRESH_DEG 45.0f  // min pitch delta to consider a raise (lowered from 55 for easier triggering)
#define RAISE_ACCEL_MIN_MG 800.0f  // acceptable accel magnitude lower bound (lowered from 850)
#define RAISE_ACCEL_MAX_MG 1200.0f // acceptable accel magnitude upper bound (increased from 1150)
#define RAISE_COOLDOWN_MS 2500     // min ms between wakeups (reduced from 3500)

static const char *TAG = "SENSORS";

static qmi8658_dev_t s_imu;
static bool s_imu_ready = false;
static volatile uint32_t s_step_count = 0; // daily steps
static sensors_activity_t s_activity = SENSORS_ACTIVITY_IDLE;
static SemaphoreHandle_t s_wom_sem = NULL; // wake-on-motion semaphore
static time_t s_last_midnight = 0;

// Debug info accessible from UI
static volatile float s_debug_ax = 0.0f;
static volatile float s_debug_ay = 0.0f;
static volatile float s_debug_az = 0.0f;
static volatile float s_debug_mag = 0.0f;
static volatile float s_debug_lp = 0.0f;

static time_t get_midnight_epoch(time_t now) {
  struct tm tm_now;
  localtime_r(&now, &tm_now);
  tm_now.tm_hour = 0;
  tm_now.tm_min = 0;
  tm_now.tm_sec = 0;
  return mktime(&tm_now);
}

static void maybe_reset_daily_counter(void) {
  time_t now = time(NULL);
  // Skip if system time not initialized (before year 2020)
  if (now < 1577836800) { // Jan 1, 2020
    return;
  }
  if (s_last_midnight == 0) {
    s_last_midnight = get_midnight_epoch(now);
    ESP_LOGI(TAG, "Daily step counter initialized, midnight epoch: %ld", (long)s_last_midnight);
  }
  time_t midnight_now = get_midnight_epoch(now);
  if (midnight_now > s_last_midnight) {
    ESP_LOGI(TAG, "Daily step counter reset at midnight (was: %lu steps)", (unsigned long)s_step_count);
    s_last_midnight = midnight_now;
    s_step_count = 0;
  }
}

static void IRAM_ATTR imu_irq_isr(void *arg) {
  BaseType_t hp = pdFALSE;
  if (s_wom_sem) {
    xSemaphoreGiveFromISR(s_wom_sem, &hp);
  }
  if (hp)
    portYIELD_FROM_ISR();
}

static esp_err_t imu_setup_irq(void) {
  gpio_config_t io = {
      .pin_bit_mask = 1ULL << IMU_IRQ_GPIO,
      .mode = GPIO_MODE_INPUT,
      // QMI8658 INT is typically active-low; use pull-up only
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      // Use falling edge to avoid interrupt storms on level changes/noise
      .intr_type = GPIO_INTR_NEGEDGE,
  };
  ESP_ERROR_CHECK(gpio_config(&io));
  esp_err_t r = gpio_install_isr_service(0);
  if (r != ESP_OK && r != ESP_ERR_INVALID_STATE) {
    return r;
  }
  // Clear any pending status before enabling
  gpio_intr_disable(IMU_IRQ_GPIO);
  (void)gpio_set_intr_type(IMU_IRQ_GPIO, GPIO_INTR_NEGEDGE);
  ESP_ERROR_CHECK(gpio_isr_handler_add(IMU_IRQ_GPIO, imu_irq_isr, NULL));
  gpio_intr_enable(IMU_IRQ_GPIO);
  return ESP_OK;
}

static bool imu_try_init_with_addr(uint8_t addr) {
  i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
  if (!bus)
    return false;
  if (qmi8658_init(&s_imu, bus, addr) != ESP_OK)
    return false;
  // Initial configuration
  // Configure for low-power step counting: accel only, ~62.5 Hz, 4g
  (void)qmi8658_enable_sensors(&s_imu, QMI8658_DISABLE_ALL);
  // Use mg units for accel magnitude
  (void)qmi8658_set_accel_range(&s_imu, QMI8658_ACCEL_RANGE_4G);
  (void)qmi8658_set_accel_odr(&s_imu, QMI8658_ACCEL_ODR_62_5HZ);
  (void)qmi8658_enable_accel(&s_imu, true);
  return true;
}

void sensors_init(void) {
  ESP_LOGI(TAG, "Initializing sensors (QMI8658)");
  if (bsp_i2c_init() != ESP_OK) {
    ESP_LOGE(TAG, "I2C not available");
    return;
  }
  // Try both possible I2C addresses
  s_imu_ready = imu_try_init_with_addr(IMU_ADDR_HIGH) ||
                imu_try_init_with_addr(IMU_ADDR_LOW);
  if (!s_imu_ready) {
    ESP_LOGE(TAG, "QMI8658 init failed");
    return;
  }
  // Create semaphore and IRQ for wake-on-motion
  s_wom_sem = xSemaphoreCreateBinary();
  if (s_wom_sem) {
    imu_setup_irq();
    // Configure wake-on-motion threshold (LSB depends on FS/ODR; empirical)
    (void)qmi8658_enable_wake_on_motion(&s_imu, 12); // ~12 LSB ~ few tens of mg
  }
  maybe_reset_daily_counter();
}

uint32_t sensors_get_step_count(void) { return s_step_count; }

sensors_activity_t sensors_get_activity(void) { return s_activity; }

void sensors_get_debug_info(float *ax, float *ay, float *az, float *mag, float *lp) {
  if (ax) *ax = s_debug_ax;
  if (ay) *ay = s_debug_ay;
  if (az) *az = s_debug_az;
  if (mag) *mag = s_debug_mag;
  if (lp) *lp = s_debug_lp;
}

void sensors_task(void *pvParameters) {
  ESP_LOGI(TAG, "Sensors task started");
  const TickType_t sample_delay_active = pdMS_TO_TICKS(20); // ~50 Hz
  const TickType_t sample_delay_idle =
      pdMS_TO_TICKS(40);     // ~25 Hz when screen off
  float lp = 0.0f;           // filtered magnitude
  const float alpha = 0.90f; // LP filter smoothing
  uint32_t last_step_ms = 0;
  uint32_t debug_counter = 0; // Debug: log every N samples
  bool first_valid_read = false; // Track first valid read to init last_step_ms
  // Ring buffer for cadence (last 8 steps)
  uint32_t step_ts_ms[8] = {0};
  int step_ts_idx = 0, step_ts_num = 0;
  // Raise-to-wake detection state
  float pitch_hist[16] = {0};
  uint32_t ts_hist[16] = {0};
  int hist_idx = 0, hist_num = 0;
  uint32_t last_raise_ms = 0;

  bool wom_enabled = true; // WoM enabled at init
  // One-shot recovery guard for bad initial readings
  bool one_shot_reconfig_done = false;
  int consecutive_bad_reads = 0;
  
  bool ready_for_next_peak = true;
  TickType_t last = xTaskGetTickCount();
  while (1) {
    maybe_reset_daily_counter();

    // If display is off, enable WoM but also sample levemente para gesto de
    // levantar pulso
    bool screen_on = display_manager_is_on();
    if (!screen_on) {
      if (!wom_enabled && s_imu_ready) {
        (void)qmi8658_enable_wake_on_motion(&s_imu, 12);
        wom_enabled = true;
      }
      /*if (s_wom_sem && xSemaphoreTake(s_wom_sem, 0) == pdTRUE) {
          ESP_LOGI(TAG, "Wake-on-motion IRQ");
          display_manager_turn_on();
          // skip rest of loop; next iter will run as screen_on
          vTaskDelay(pdMS_TO_TICKS(50));
          continue;
      }*/
    }

    if (!s_imu_ready) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    // Display is on: prefer normal sampling, disable WoM to avoid extra
    // interrupts
    if (screen_on && wom_enabled) {
      ESP_LOGI(TAG, "Disabling WoM, switching to normal mode");
      esp_err_t ret = qmi8658_disable_wake_on_motion(&s_imu);
      ESP_LOGI(TAG, "WoM disable result: %s", esp_err_to_name(ret));
      
      // Re-enable accelerometer in normal mode after disabling WoM
      ret = qmi8658_set_accel_range(&s_imu, QMI8658_ACCEL_RANGE_4G);
      ESP_LOGI(TAG, "Set accel range result: %s", esp_err_to_name(ret));
      
      ret = qmi8658_set_accel_odr(&s_imu, QMI8658_ACCEL_ODR_62_5HZ);
      ESP_LOGI(TAG, "Set accel ODR result: %s", esp_err_to_name(ret));
      
      ret = qmi8658_enable_accel(&s_imu, true);
      ESP_LOGI(TAG, "Enable accel result: %s", esp_err_to_name(ret));
      
      wom_enabled = false;
      consecutive_bad_reads = 0; // Reset since we just reconfigured
    }

    float ax = 0, ay = 0, az = 0;
    float mag = 0.0f;  // Declare outside so it's available for raise-to-wake
    esp_err_t read_result = qmi8658_read_accel(&s_imu, &ax, &ay, &az);
    if (read_result == ESP_OK) {
      // Detect unrealistic all-zero readings at startup
      if (ax == 0.0f && ay == 0.0f && az == 0.0f) {
        consecutive_bad_reads++;
      } else {
        consecutive_bad_reads = 0;
        // Initialize last_step_ms on first valid non-zero read
        if (!first_valid_read) {
          uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
          last_step_ms = now_ms;
          first_valid_read = true;
          ESP_LOGI(TAG, "Step detection initialized: t=%lu ax=%.1f ay=%.1f az=%.1f", 
                   (unsigned long)now_ms, ax, ay, az);
        }
      }
      // ax,ay,az in mg
      mag = sqrtf(ax * ax + ay * ay + az * az); // mg
      float hp = mag - 1000.0f;                       // remove gravity
      lp = alpha * lp + (1.0f - alpha) * hp;
      
      // Update debug info for UI
      s_debug_ax = ax;
      s_debug_ay = ay;
      s_debug_az = az;
      s_debug_mag = mag;
      s_debug_lp = lp;

      // Debug logging every 2 seconds
      debug_counter++;
      if (debug_counter >= 100) {  // Every ~2s at 50Hz
        ESP_LOGI(TAG, "Sensor: ax=%.1f ay=%.1f az=%.1f mag=%.1f lp=%.1f steps=%lu ready=%d", 
                 ax, ay, az, mag, lp, (unsigned long)s_step_count, ready_for_next_peak);
        debug_counter = 0;
      }
    } else {
      // Log read errors occasionally
      consecutive_bad_reads++;
      debug_counter++;
      if (debug_counter >= 100) {
        ESP_LOGE(TAG, "Failed to read accel: %s", esp_err_to_name(read_result));
        debug_counter = 0;
      }
    }

    // One-shot recovery reconfigure if initial reads look bad while screen is on
    if (screen_on && !one_shot_reconfig_done && consecutive_bad_reads >= 3) {
      ESP_LOGW(TAG, "Accel one-shot reconfigure due to bad startup reads (%d)", consecutive_bad_reads);
      (void)qmi8658_disable_wake_on_motion(&s_imu);
      (void)qmi8658_set_accel_range(&s_imu, QMI8658_ACCEL_RANGE_4G);
      (void)qmi8658_set_accel_odr(&s_imu, QMI8658_ACCEL_ODR_62_5HZ);
      (void)qmi8658_enable_accel(&s_imu, true);
      wom_enabled = false;
      one_shot_reconfig_done = true;
      consecutive_bad_reads = 0;
    }

    // Only process step detection if we got valid data
    if (read_result == ESP_OK) {

      // Peak detection
      const float THRESH = 40.0f; // mg - tuned threshold
      uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
      
      // Calculate dt only if initialized
      if (!first_valid_read) {
        // Skip step detection until we have valid baseline
        continue;
      }
      
      uint32_t dt = now_ms - last_step_ms;
      
      // If too much time has passed, reset the baseline to allow fresh detection
      if (dt > 2000) {
        last_step_ms = now_ms;
        dt = 0;
      }
      
      if (lp > THRESH && dt > 280 && dt < 2000) {
        if (ready_for_next_peak) {
          s_step_count++;
          ESP_LOGI(TAG, "✓ STEP #%lu! lp=%.1f dt=%lums", 
                   (unsigned long)s_step_count, lp, (unsigned long)dt);
          // cadence buffer
          step_ts_ms[step_ts_idx] = now_ms;
          step_ts_idx = (step_ts_idx + 1) & 7;
          if (step_ts_num < 8)
            step_ts_num++;
          last_step_ms = now_ms;
          ready_for_next_peak = false;
        } else {
          // Log why step was blocked
          static uint32_t last_blocked_log = 0;
          if (now_ms - last_blocked_log > 500) {
            ESP_LOGI(TAG, "Step blocked: not ready (lp=%.1f, need to drop below %.1f)", 
                     lp, THRESH * 0.5f);
            last_blocked_log = now_ms;
          }
        }
      } else if (lp < THRESH * 0.5f) {
        if (!ready_for_next_peak) {
          ESP_LOGI(TAG, "Ready for next step: lp=%.1f dropped below %.1f", lp, THRESH * 0.5f);
          // Reset timing baseline when becoming ready
          last_step_ms = now_ms;
        }
        ready_for_next_peak = true;
      } else if (lp > THRESH) {
        // Above threshold but timing wrong
        static uint32_t last_timing_log = 0;
        if (now_ms - last_timing_log > 500) {
          ESP_LOGI(TAG, "Peak timing wrong: lp=%.1f dt=%lums (need 280-2000)", 
                   lp, (unsigned long)dt);
          last_timing_log = now_ms;
        }
      }

      // Classify activity by cadence (last N steps)
      if (step_ts_num >= 2) {
        uint32_t oldest = step_ts_ms[(step_ts_idx - step_ts_num + 8) & 7];
        uint32_t newest = step_ts_ms[(step_ts_idx - 1 + 8) & 7];
        uint32_t span_ms = newest - oldest;
        float spm = 0.0f;
        if (span_ms > 0) {
          spm = 60000.0f * (float)(step_ts_num - 1) / (float)span_ms;
        }
        if (spm > 130.0f)
          s_activity = SENSORS_ACTIVITY_RUN;
        else if (spm > 60.0f)
          s_activity = SENSORS_ACTIVITY_WALK;
        else if (spm > 10.0f)
          s_activity = SENSORS_ACTIVITY_OTHER;
        else
          s_activity = SENSORS_ACTIVITY_IDLE;
      } else {
        s_activity = SENSORS_ACTIVITY_IDLE;
      }

      // Raise-to-wake: compute pitch angle from accel (degrees)
      // pitch ~ rotation around Y: -ax against gravity
      float ax_g = ax / 1000.0f, ay_g = ay / 1000.0f, az_g = az / 1000.0f;
      float pitch = (float)(atan2f(-ax_g, sqrtf(ay_g * ay_g + az_g * az_g)) *
                            180.0f / (float)M_PI);
      // uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
      pitch_hist[hist_idx] = pitch;
      ts_hist[hist_idx] = now_ms;
      hist_idx = (hist_idx + 1) & 15;
      if (hist_num < 16)
        hist_num++;

      if (!screen_on) {
        // Compare with sample ~400-600ms atrás
        float pitch_prev = pitch;
        for (int k = 1; k <= hist_num; ++k) {
          int idx = (hist_idx - k + 16) & 15;
          uint32_t dtms = now_ms - ts_hist[idx];
          if (dtms >= 400 && dtms <= 700) {
            pitch_prev = pitch_hist[idx];
            break;
          }
        }
        float dp = pitch - pitch_prev; // positive when lifting display up
        bool accel_ok = (mag > RAISE_ACCEL_MIN_MG &&
                         mag < RAISE_ACCEL_MAX_MG); // avoid big shakes
        bool cooldown_ok = (now_ms - last_raise_ms) > RAISE_COOLDOWN_MS;
        if (dp > RAISE_DP_THRESH_DEG && accel_ok && cooldown_ok) {
          ESP_LOGI(TAG, "Raise-to-wake: dp=%.1f pitch=%.1f prev=%.1f", dp,
                   pitch, pitch_prev);
          last_raise_ms = now_ms;
          display_manager_turn_on();
          // let next iter process as screen_on
        }
      }
    }
    
    TickType_t delay = screen_on ? sample_delay_active : sample_delay_idle;
    vTaskDelayUntil(&last, delay);
  }
}
