#include <stdio.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#include <app_event_manager.h>

#define MODULE main
#include <caf/events/module_state_event.h>
#include <caf/events/button_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);


#define LED0_NODE DT_ALIAS(myled0alias)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);


void main(void)
{
	int ret;
	struct sensor_value val;
	
	const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	const struct device *const qdec = DEVICE_DT_GET(DT_NODELABEL(qdec));


	if (app_event_manager_init()) {
		LOG_ERR("Event manager not initialized");
	} else {
		module_set_state(MODULE_STATE_READY);
	}

	if (!device_is_ready(led.port)) {
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}

	if (!device_is_ready(qdec)) {
		LOG_ERR("Qdec device is not ready\n");
		return;
	}

	while (true) {
		ret = sensor_sample_fetch(qdec);
		if (ret != 0) {
			LOG_ERR("Failed to fetch sample (%d)\n", ret);
			return;
		}

		ret = sensor_channel_get(qdec, SENSOR_CHAN_ROTATION, &val);
		if (ret != 0) {
			LOG_ERR("Failed to get data (%d)\n", ret);
			return;
		}

		LOG_INF("Position = %d degrees", val.val1);

		k_msleep(1000);
	}

	LOG_INF("****************************************");
	LOG_INF("MAIN DONE");
	LOG_INF("****************************************");

	k_sleep(K_SECONDS(3));
	pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);

	LOG_INF("PM_DEVICE_ACTION_SUSPEND");
}

static bool event_handler(const struct app_event_header *eh)
{
	int ret;
	const struct button_event *evt;

	if (is_button_event(eh)) {
		evt = cast_button_event(eh);

		if (evt->pressed) {
			LOG_INF("Pin Toggle");
			ret = gpio_pin_toggle_dt(&led);
		}
	}

	return true;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, button_event);
