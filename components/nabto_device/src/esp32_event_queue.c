#include "esp32_event_queue.h"

#include <modules/event_queue/nm_event_queue.h>

#include <platform/np_timestamp_wrapper.h>
#include <platform/np_allocator.h>
#include <platform/np_logging.h>

#include "esp_pthread.h"

#define LOG NABTO_LOG_MODULE_EVENT_QUEUE

struct np_event {
    struct esp32_event_queue* queue;
    struct nm_event_queue_event event;
};

static np_error_code create_event(struct np_event_queue* obj, np_event_callback cb, void* cbData, struct np_event** event);
static void destroy_event(struct np_event* event);
static void post_event(struct np_event* event);
static bool post_event_maybe_double(struct np_event* event);

static void cancel_event(struct np_event* event);

static void post_timed_event(struct np_event* event, uint32_t milliseconds);

static void* queue_thread(void* data);

static struct np_event_queue_functions module = {
    .create = &create_event,
    .destroy = &destroy_event,
    .post = &post_event,
    .post_maybe_double = &post_event_maybe_double,
    .cancel = &cancel_event,
    .post_timed = &post_timed_event,
};

struct np_event_queue esp32_event_queue_get_impl(struct esp32_event_queue* queue)
{
    struct np_event_queue eq;
    eq.mptr = &module;
    eq.data = queue;
    return eq;
}

void esp32_event_queue_init(struct esp32_event_queue* queue, struct nabto_device_mutex* coreMutex, struct np_timestamp* ts)
{
    nm_event_queue_init(&queue->eventQueue);
    queue->stopped = false;
    queue->coreMutex = coreMutex;
    queue->ts = *ts;
    queue->queueMutex = nabto_device_threads_create_mutex();
    queue->condition = nabto_device_threads_create_condition();
}

static size_t EVENT_QUEUE_THREAD_STACK_SIZE = CONFIG_NABTO_DEVICE_EVENT_QUEUE_THREAD_STACK_SIZE;


np_error_code esp32_event_queue_run(struct esp32_event_queue* queue)
{
    nabto_device_threads_mutex_lock(queue->queueMutex);

    esp_pthread_cfg_t pthreadConfig = esp_pthread_get_default_config();
    pthreadConfig.stack_size = EVENT_QUEUE_THREAD_STACK_SIZE;
    pthreadConfig.thread_name = "EventQueue";
    // Possible make these configurable
    pthreadConfig.prio = 25;
    //pthreadConfig.pin_to_core = 1;

    esp_err_t err = esp_pthread_set_cfg(&pthreadConfig);
    if (err != ESP_OK) {
        NABTO_LOG_ERROR(LOG, "Cannot set pthread cfg");
    }

    int ec = pthread_create(&queue->queueThread, NULL, queue_thread, queue);

    nabto_device_threads_mutex_unlock(queue->queueMutex);
    if (ec == 0) {
        return NABTO_EC_OK;
    } else {
        NABTO_LOG_ERROR(LOG, "Cannot create event queue thread %d", ec);
        return NABTO_EC_UNKNOWN;
    }
}

void esp32_event_queue_deinit(struct esp32_event_queue* queue)
{
    nabto_device_threads_mutex_lock(queue->queueMutex);
    // stop queue
    nabto_device_threads_mutex_unlock(queue->queueMutex);
    nabto_device_threads_free_cond(queue->condition);
    nabto_device_threads_free_mutex(queue->queueMutex);
}



void esp32_event_queue_stop_blocking(struct esp32_event_queue* queue)
{
    nabto_device_threads_mutex_lock(queue->queueMutex);
    queue->stopped = true;
    nabto_device_threads_mutex_unlock(queue->queueMutex);
    nabto_device_threads_cond_signal(queue->condition);
    void* retval;
    pthread_join(queue->queueThread, &retval);
}


np_error_code create_event(struct np_event_queue* obj, np_event_callback cb, void* cbData, struct np_event** event)
{
    struct np_event* ev = np_calloc(1, sizeof(struct np_event));
    if (ev == NULL) {
        return NABTO_EC_OUT_OF_MEMORY;
    }
    nm_event_queue_event_init(&ev->event, cb, cbData);
    ev->queue = obj->data;
    *event = ev;
    return NABTO_EC_OK;
}

void destroy_event(struct np_event* event)
{
    struct esp32_event_queue* eq = event->queue;
    nabto_device_threads_mutex_lock(eq->queueMutex);
    nm_event_queue_event_deinit(&event->event);
    np_free(event);
    nabto_device_threads_mutex_unlock(eq->queueMutex);
}

void post_event(struct np_event* event)
{
    struct esp32_event_queue* queue = event->queue;
    nabto_device_threads_mutex_lock(queue->queueMutex);
    nm_event_queue_post_event(&queue->eventQueue, &event->event);
    nabto_device_threads_mutex_unlock(queue->queueMutex);
    nabto_device_threads_cond_signal(queue->condition);
}

bool post_event_maybe_double(struct np_event* event)
{
    struct esp32_event_queue* queue = event->queue;
    bool status;
    nabto_device_threads_mutex_lock(queue->queueMutex);
    status = nm_event_queue_post_event_maybe_double(&queue->eventQueue, &event->event);
    nabto_device_threads_mutex_unlock(queue->queueMutex);
    nabto_device_threads_cond_signal(queue->condition);
    return status;
}

void cancel_event(struct np_event* event)
{
    struct esp32_event_queue* queue = event->queue;
    nabto_device_threads_mutex_lock(queue->queueMutex);
    nm_event_queue_cancel_event(&event->event);
    nabto_device_threads_mutex_unlock(queue->queueMutex);
}

void post_timed_event(struct np_event* event, uint32_t milliseconds)
{
    struct esp32_event_queue* queue = event->queue;

    uint32_t now = np_timestamp_now_ms(&queue->ts);
    uint32_t timestamp = now + milliseconds;
    nabto_device_threads_mutex_lock(queue->queueMutex);
    nm_event_queue_post_timed_event(&queue->eventQueue, &event->event, timestamp);
    nabto_device_threads_mutex_unlock(queue->queueMutex);
    nabto_device_threads_cond_signal(queue->condition);
}

void* queue_thread(void* data)
{
    struct esp32_event_queue* queue = data;
    while(true) {
        uint32_t nextEvent;
        uint32_t now = np_timestamp_now_ms(&queue->ts);
        struct nm_event_queue_event* event = NULL;



        nabto_device_threads_mutex_lock(queue->queueMutex);
        if (nm_event_queue_take_event(&queue->eventQueue, &event)) {
            // ok execute the event later.
        } else if (nm_event_queue_take_timed_event(&queue->eventQueue, now, &event)) {
            // ok execute the event later.
        } else if (nm_event_queue_next_timed_event(&queue->eventQueue, &nextEvent)) {
            int32_t diff = np_timestamp_difference(nextEvent, now);
            // ok wait for event to become ready
            nabto_device_threads_cond_timed_wait(queue->condition, queue->queueMutex, diff);
        } else if (queue->stopped) {
            nabto_device_threads_mutex_unlock(queue->queueMutex);
            return NULL;
        } else {
            nabto_device_threads_cond_wait(queue->condition, queue->queueMutex);
        }

        nabto_device_threads_mutex_unlock(queue->queueMutex);


        if (event != NULL) {
            nabto_device_threads_mutex_lock(queue->coreMutex);
            event->cb(event->data);
            nabto_device_threads_mutex_unlock(queue->coreMutex);
        }


    }
    return NULL;
}
