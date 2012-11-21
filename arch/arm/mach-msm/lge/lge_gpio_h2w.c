/*
 *  H2W device detection driver.
 *
 * Copyright (C) 2008 -2011 LGE Corporation.
 * Copyright (C) 2008 Google, Inc.
 * 
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/
#include <linux/module.h>
#include <linux/sysdev.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/debugfs.h>
#include <asm/gpio.h>
#include <asm/atomic.h>
#include <mach/board.h>
#include <mach/vreg.h>
#include <mach/board_lge.h>  
#include <linux/slab.h>
#include <linux/wakelock.h>

#define DEBUG_H2W
#ifdef DEBUG_H2W
#define H2W_DBG(fmt, arg...) printk(KERN_INFO "[H2W] %s " fmt "\n", __FUNCTION__, ## arg)
#else
#define H2W_DBG(fmt, arg...) do {} while (0)
#endif  

static struct workqueue_struct *g_detection_work_queue;
static void detection_work(struct work_struct *work);
static DECLARE_WORK(g_detection_work, detection_work);
static int ip_dev_reg;

/*Unplug State*/
#define BIT_NO_DEVICE				0
/*4Pole Headset - headset with mic*/
#define BIT_HEADSET					(1 << 0)
/*3Pole Headset - headset without mic*/
#define BIT_HEADSET_SPEAKER_ONLY	(1 << 1) 

struct h2w_info {
	struct switch_dev sdev;
	struct input_dev *input;
	
	/*GPIO Number - Headset Detect */
	int gpio_detect;
	/*GPIO Number - Headset Button Detect */	
	int gpio_button_detect;
	/*GPIO Number - Headset Pole Detect */	
	int gpio_jpole;
#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT	
	/*GPIO Number - Mic Bias Enable*/	
	int gpio_mic_bias_en;
#endif
	void (*mic_bias_power)(int);

	/*Button Down State*/
	atomic_t btn_state;
	/*Button Check Skip*/
	int ignore_btn;

	/*Headset Detect*/
	unsigned int irq;
	/*Headset Button Detect*/	
	unsigned int irq_btn;

	/*Headset Detect Timer*/		
	struct hrtimer timer;
	ktime_t debounce_time;
	ktime_t unplug_debounce_time;

	/*Headset Button Detect Timer*/		
	struct hrtimer btn_timer;
	ktime_t btn_debounce_time;

	/*Wake Lock/Unlock*/		
	struct wake_lock wake_lock;

	/*State*/
	int enable_btn_irq;
};

static struct h2w_info *hi;

static ssize_t gpio_h2w_print_name(struct switch_dev *sdev, char *buf)
{
	switch (switch_get_state(&hi->sdev)) 
	{
		case BIT_NO_DEVICE:
			return sprintf(buf, "No Device\n");
		case BIT_HEADSET:
		case BIT_HEADSET_SPEAKER_ONLY:		
			return sprintf(buf, "Headset\n");
	}
	return -EINVAL;
}

static void button_pressed(void)
{
	H2W_DBG("");
	
	atomic_set(&hi->btn_state, 1);
	input_report_key(hi->input, KEY_MEDIA, 1);
	input_sync(hi->input);
}

static void button_released(void)
{
	H2W_DBG("");
	
	atomic_set(&hi->btn_state, 0);
	input_report_key(hi->input, KEY_MEDIA, 0);
	input_sync(hi->input);
}

static void insert_headset(void)
{
	unsigned long irq_flags;
	int jpole;

	if(hi->mic_bias_power){
		hi->mic_bias_power(1);
	}
#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT
	gpio_set_value(hi->gpio_mic_bias_en, 1);
	msleep(100);
#endif 	
	jpole = gpio_get_value(hi->gpio_jpole);

	 /* 3pole */
	if(jpole)
   	{	
		H2W_DBG("insert_headset 3pole(%d) \n",jpole);

   		hi->ignore_btn = 1;
		/*Report Headset State*/
		switch_set_state(&hi->sdev, BIT_HEADSET_SPEAKER_ONLY);
#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT			
		gpio_set_value(hi->gpio_mic_bias_en, 0);
#endif
		if(hi->mic_bias_power){
			hi->mic_bias_power(0);
		}
	}
	/* 4pole */	
	else
	{
		H2W_DBG("insert_headset 4pole(%d) \n",jpole);

		hi->ignore_btn =0;
		/*Report Headset State*/
		switch_set_state(&hi->sdev, BIT_HEADSET);

		/* Enable button irq */
		local_irq_save(irq_flags);
		enable_irq(hi->irq_btn);
		set_irq_wake(hi->irq_btn, 1);
		hi->enable_btn_irq = 1;
		local_irq_restore(irq_flags);

	}
	hi->debounce_time = ktime_set(0, 20000000);  /* 20 ms */
	hi->unplug_debounce_time = ktime_set(0, 100000000);  /* 100 ms */ 
}

static void remove_headset(void)
{
	unsigned long irq_flags;

	H2W_DBG("");

	/*Report Headset State*/
	input_report_switch(hi->input, SW_HEADPHONE_INSERT, 0);
	switch_set_state(&hi->sdev, BIT_NO_DEVICE);
	input_sync(hi->input);

	/* Disable button */
	if(hi->enable_btn_irq)
	{
		local_irq_save(irq_flags);
		disable_irq_nosync(hi->irq_btn);
		set_irq_wake(hi->irq_btn, 0);
		hi->enable_btn_irq = 0;
		local_irq_restore(irq_flags);
	}
	/*Check Button State*/
	if (atomic_read(&hi->btn_state))
		button_released();
	
#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT
	gpio_set_value(hi->gpio_mic_bias_en, 0);
#endif 
	if(hi->mic_bias_power){
		hi->mic_bias_power(0);
	}

	hi->debounce_time = ktime_set(0, 500000000);  /* 500ms */
}

static void detection_work(struct work_struct *work)
{
	/*Headset detach*/
	if (gpio_get_value(hi->gpio_detect) == 1) {
		if (switch_get_state(&hi->sdev) == BIT_HEADSET
			|| switch_get_state(&hi->sdev) == BIT_HEADSET_SPEAKER_ONLY
		){
			remove_headset();
		}
	}
	/*Headset attach*/
	else{
		if (switch_get_state(&hi->sdev) == BIT_NO_DEVICE){
			insert_headset();
		}
	}
}

static enum hrtimer_restart button_event_timer_func(struct hrtimer *data)
{
	int headset_in  =0;
	int btn_down  =0;

	H2W_DBG("");

	/* Low Detect, Fast Check*/
	/* "button timer < headset detach timer" makes abnormal operation*/
	headset_in	= gpio_get_value(hi->gpio_detect)?0: 1;
	/*Headset button - High Detect*/
	btn_down	= gpio_get_value(hi->gpio_button_detect)?1 :0; 	

	if(hi->ignore_btn){
		H2W_DBG("ignore_btn");
		return HRTIMER_NORESTART;
	}
	
	if ((switch_get_state(&hi->sdev) == BIT_HEADSET)  && headset_in){
		if (btn_down){
			if (!atomic_read(&hi->btn_state))
				button_pressed();
			
		} else {
			if (atomic_read(&hi->btn_state))
				button_released();
		}
	}

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart detect_event_timer_func(struct hrtimer *data)
{
	queue_work(g_detection_work_queue, &g_detection_work);
	return HRTIMER_NORESTART;
}

static irqreturn_t detect_irq_handler(int irq, void *dev_id)
{
	int value1, value2;
	int retry_limit = 10;
	
	set_irq_type(hi->irq_btn, IRQF_TRIGGER_HIGH);
	
	do {
		value1 = gpio_get_value(hi->gpio_detect);
		set_irq_type(hi->irq, value1 ?IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
		value2 = gpio_get_value(hi->gpio_detect);
	} while (value1 != value2 && retry_limit-- > 0);

	H2W_DBG("value2 = %d (%d retries)", value2, (10-retry_limit));
	
	wake_lock_timeout(&hi->wake_lock, 2*HZ); // 2 second
		
	/*Attached*/	
	if (switch_get_state(&hi->sdev) == BIT_NO_DEVICE) {
		hrtimer_start(&hi->timer, hi->debounce_time, HRTIMER_MODE_REL); 
	}
	/*Detached*/		
	else if(switch_get_state(&hi->sdev) == BIT_HEADSET
			|| switch_get_state(&hi->sdev) == BIT_HEADSET_SPEAKER_ONLY
	){
#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT
		gpio_set_value(hi->gpio_mic_bias_en, 0);
#endif 		
		hrtimer_start(&hi->timer, hi->unplug_debounce_time, HRTIMER_MODE_REL);
	}

	return IRQ_HANDLED;
}

static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
	int value1, value2;
	int retry_limit = 10;

	do {
		value1 = gpio_get_value(hi->gpio_button_detect);
		set_irq_type(hi->irq_btn, value1 ?IRQF_TRIGGER_LOW : IRQF_TRIGGER_HIGH);
		value2 = gpio_get_value(hi->gpio_button_detect);
	} while (value1 != value2 && retry_limit-- > 0);

	H2W_DBG("value2 = %d (%d retries)", value2, (10-retry_limit));

	hrtimer_start(&hi->btn_timer, hi->btn_debounce_time, HRTIMER_MODE_REL);

	return IRQ_HANDLED;
}

static int gpio_h2w_probe(struct platform_device *pdev)
{
	int ret;
	struct gpio_h2w_platform_data *pdata = pdev->dev.platform_data;

	H2W_DBG("H2W: Registering H2W (headset) driver\n");
	hi = kzalloc(sizeof(struct h2w_info), GFP_KERNEL);
	if (!hi)
		return -ENOMEM;

	atomic_set(&hi->btn_state, 0);
	hi->ignore_btn = 0;
	hi->mic_bias_power = pdata->mic_bias_power;
	hi->debounce_time = ktime_set(0, 500000000);   /* VS760 100 ms -> 300ms */
	hi->btn_debounce_time = ktime_set(0, 100000000); /* 10 ms */
	hi->unplug_debounce_time = ktime_set(0, 100000000); //add eklee 100 ms 

	hi->gpio_detect = pdata->gpio_detect;    
	hi->gpio_button_detect = pdata->gpio_button_detect;
	hi->gpio_jpole = pdata->gpio_jpole;
#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT		
	hi->gpio_mic_bias_en = pdata->gpio_mic_bias_en;
#endif
	hi->sdev.name = "h2w";
	hi->sdev.print_name = gpio_h2w_print_name;

	wake_lock_init(&hi->wake_lock, WAKE_LOCK_SUSPEND, "h2w_detect_lock");

	ret = switch_dev_register(&hi->sdev);
	if (ret < 0)
		goto err_switch_dev_register;

	g_detection_work_queue = create_workqueue("detection");
	if (g_detection_work_queue == NULL) {
		ret = -ENOMEM;
		goto err_create_work_queue;
	}

	ret = gpio_request(hi->gpio_detect, "h2w_detect");
	if (ret < 0)
		goto err_request_detect_gpio;

	ret = gpio_request(hi->gpio_button_detect, "h2w_button");
	if (ret < 0)
		goto err_request_button_gpio;

	ret = gpio_request(hi->gpio_jpole, "h2w_jpole");
	if (ret < 0)
		goto err_request_jpole_gpio;

#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT		
	ret = gpio_request(hi->gpio_mic_bias_en, "h2w_mic_bias_en");
	if (ret < 0)
		goto err_request_mic_bias_en_gpio;
#endif

	ret = gpio_direction_input(hi->gpio_detect);
	if (ret < 0)
		goto err_set_detect_gpio;

	ret = gpio_direction_input(hi->gpio_button_detect);
	if (ret < 0)
		goto err_set_button_gpio;

	ret = gpio_direction_input(hi->gpio_jpole);
	if (ret < 0)
		goto err_set_jpole_gpio;

#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT	
	ret = gpio_direction_output(hi->gpio_mic_bias_en,0);
	if (ret < 0)
		goto err_set_mic_bias_en_gpio;
#endif

	hi->irq = gpio_to_irq(hi->gpio_detect);
	if (hi->irq < 0) {
		ret = hi->irq;
		goto err_get_h2w_detect_irq_num_failed;
	}

	hi->irq_btn = gpio_to_irq(hi->gpio_button_detect);
	if (hi->irq_btn < 0) {
		ret = hi->irq_btn;
		goto err_get_button_irq_num_failed;
	}

#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT	
	gpio_set_value(hi->gpio_mic_bias_en, 0);
#endif

//	hrtimer_init(&hi->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrtimer_init(&hi->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	hi->timer.function = detect_event_timer_func;
	hrtimer_init(&hi->btn_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hi->btn_timer.function = button_event_timer_func;

	ret = request_irq(hi->irq, detect_irq_handler,
			  IRQF_TRIGGER_LOW, "h2w_detect", NULL); 
	if (ret < 0)
		goto err_request_detect_irq;

	/* Disable button until plugged in */
	hi->enable_btn_irq = 0;
	set_irq_flags(hi->irq_btn, IRQF_VALID | IRQF_NOAUTOEN);
	ret = request_irq(hi->irq_btn, button_irq_handler,
			  IRQF_TRIGGER_HIGH, "h2w_button", NULL);
	if (ret < 0)
		goto err_request_h2w_headset_button_irq;

	ret = set_irq_wake(hi->irq, 1);
	if (ret < 0)
		goto err_request_input_dev;

	hi->input = input_allocate_device();
	if (!hi->input) {
		ret = -ENOMEM;
		goto err_request_input_dev;
	}

	hi->input->name = "h2w headset";
	hi->input->evbit[0] = BIT_MASK(EV_KEY);
	hi->input->keybit[BIT_WORD(KEY_MEDIA)] = BIT_MASK(KEY_MEDIA);

	ret = input_register_device(hi->input);
	if (ret < 0)
		goto err_register_input_dev;
	
	ip_dev_reg = 1;

	/* check the inital state of headset */
	queue_work(g_detection_work_queue, &g_detection_work);

	return 0;
	
err_register_input_dev:
	input_free_device(hi->input);
err_request_input_dev:
	free_irq(hi->irq_btn, 0);
err_request_h2w_headset_button_irq:
	free_irq(hi->irq, 0);
err_request_detect_irq:
err_get_button_irq_num_failed:
err_get_h2w_detect_irq_num_failed:
err_set_button_gpio:
err_set_detect_gpio:
err_set_jpole_gpio:		
err_set_mic_bias_en_gpio:
	gpio_free(hi->gpio_button_detect);
err_request_button_gpio:
	gpio_free(hi->gpio_detect);

#ifdef CONFIG_LGE_AUDIO_HEADSET_PROTECT	
err_request_mic_bias_en_gpio:
	gpio_free(hi->gpio_mic_bias_en);
#endif

err_request_jpole_gpio:
	gpio_free(hi->gpio_jpole);

err_request_detect_gpio:
	destroy_workqueue(g_detection_work_queue);
err_create_work_queue:
	switch_dev_unregister(&hi->sdev);
err_switch_dev_register:
	printk(KERN_ERR "H2W: Failed to register driver\n");

	wake_lock_destroy(&hi->wake_lock);
	
	return ret;
}

static int gpio_h2w_remove(struct platform_device *pdev)
{
	H2W_DBG("");
	if (switch_get_state(&hi->sdev))
		remove_headset();
	input_unregister_device(hi->input);
	gpio_free(hi->gpio_button_detect);
	gpio_free(hi->gpio_detect);
	free_irq(hi->irq_btn, 0);
	free_irq(hi->irq, 0);
	destroy_workqueue(g_detection_work_queue);
	switch_dev_unregister(&hi->sdev);
	ip_dev_reg = 0;
	wake_lock_destroy(&hi->wake_lock);
	return 0;
}

static struct platform_driver gpio_h2w_driver = {
	.probe		= gpio_h2w_probe,
	.remove		= gpio_h2w_remove,
	.driver		= {
		.name		= "gpio-h2w",
		.owner		= THIS_MODULE,
	},
};

static int __init gpio_h2w_init(void)
{
	H2W_DBG("");
	return platform_driver_register(&gpio_h2w_driver);
}

static void __exit gpio_h2w_exit(void)
{
	platform_driver_unregister(&gpio_h2w_driver);
}

module_init(gpio_h2w_init);
module_exit(gpio_h2w_exit);

MODULE_AUTHOR("LG Electronics");
MODULE_DESCRIPTION("LGE 2 Wire detection driver");
MODULE_LICENSE("GPL");
