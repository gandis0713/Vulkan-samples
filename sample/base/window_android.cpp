#include "window.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>

namespace jipu
{

static void onAppCmd(android_app* app, int32_t cmd)
{
    Window* window = static_cast<Window*>(app->userData);
    switch (cmd)
    {
    case APP_CMD_INIT_WINDOW:
        window->setWidth(ANativeWindow_getWidth(app->window));
        window->setHeight(ANativeWindow_getHeight(app->window));
        window->init();
        break;
    case APP_CMD_TERM_WINDOW:
        // TODO: delete JIPU
        break;
    default:
        break;
    }
}

Window::Window(const WindowDescriptor& descriptor)
    : m_handle(descriptor.handle)
    , m_width(descriptor.width)
    , m_height(descriptor.height)
{
    struct android_app* app = static_cast<android_app*>(m_handle);

    // Set the callback to process system events
    app->userData = this;
    app->onAppCmd = onAppCmd;
}

Window::~Window()
{
    // do nothing.
}

int Window::exec()
{
    android_app* app = static_cast<android_app*>(m_handle);

    int events;
    android_poll_source* source;

    // Main loop
    do
    {
        auto result = ALooper_pollOnce(0, nullptr, &events, (void**)&source);
        if (result == ALOOPER_POLL_ERROR)
        {
            throw std::runtime_error("Failed to poll once for android.");
        }

        if (source != nullptr)
            source->process(app, source);

        if (m_initialized)
        {
            android_input_buffer* inputBuffer = android_app_swap_input_buffers(app);
            if (inputBuffer && inputBuffer->motionEventsCount)
            {
                for (uint64_t i = 0; i < inputBuffer->motionEventsCount; ++i)
                {
                    GameActivityMotionEvent* motionEvent = &inputBuffer->motionEvents[i];

                    if (motionEvent->pointerCount > 0)
                    {
                        const int action = motionEvent->action;
                        const int actionMasked = action & AMOTION_EVENT_ACTION_MASK;
                        // Initialize pointerIndex to the max size, we only cook an
                        // event at the end of the function if pointerIndex is set to a valid index range
                        uint32_t pointerIndex = GAMEACTIVITY_MAX_NUM_POINTERS_IN_MOTION_EVENT;

                        switch (actionMasked)
                        {
                        case AMOTION_EVENT_ACTION_DOWN:
                            pointerIndex = 0;
                            m_leftMouseButton = true;
                            break;
                        case AMOTION_EVENT_ACTION_POINTER_DOWN:
                            pointerIndex = ((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                            m_leftMouseButton = true;
                            break;
                        case AMOTION_EVENT_ACTION_UP:
                            pointerIndex = 0;
                            m_leftMouseButton = false;
                            break;
                        case AMOTION_EVENT_ACTION_POINTER_UP:
                            pointerIndex = ((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
                            m_leftMouseButton = false;
                            break;
                        case AMOTION_EVENT_ACTION_MOVE:
                            pointerIndex = 0;
                            break;
                        default:
                            break;
                        }

                        if (pointerIndex != GAMEACTIVITY_MAX_NUM_POINTERS_IN_MOTION_EVENT)
                        {
                            auto& pointer = motionEvent->pointers[pointerIndex];
                            m_mouseX = GameActivityPointerAxes_getX(&pointer);
                            m_mouseY = GameActivityPointerAxes_getY(&pointer);
                        }
                    }
                }
                android_app_clear_motion_events(inputBuffer);
            }
            update();
            draw();
        }

    } while (app->destroyRequested == 0);

    return 0;
}

void* Window::getWindowHandle()
{
    struct android_app* app = static_cast<android_app*>(m_handle);
    return static_cast<void*>(app->window);
}

} // namespace jipu