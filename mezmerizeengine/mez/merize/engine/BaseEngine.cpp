
#include "BaseEngine.h"
#include <SFML/Graphics.hpp>
//TODO: need to use gtk
#include <Windows.h>
#include "mez/merize/version.h"
#include <stdio.h>
#include <iostream>
#include "mez/merize/helpers/static_format.h"
#include "eng_console.h"
#include "mez/merize/entityprivate/efactory.h"
#include "mez/merize/ren/rinterface/rstatic.h"
#include "mez/merize/io/input.h"
#include <mez/merize/ren/rinterface/rinterface_def.h>
#include <mez/merize/stock/stock_conball.h>
#include <mez/merize/stock/stock_entball.h>


//holy what a mess

//this system sucks
#define ALLOW_RENDERING RENDERABLE_MODE && !m_launchparameters.m_textmode
#define FORBID_RENDERING !(ALLOW_RENDERING)

BaseEngine* engine;
EngineVersion engine_version;
#pragma warning (push)
#pragma warning (disable: 26495)
BaseEngine::BaseEngine(EngineLaunchParameters launchparams)
{
	m_launchparameters = launchparams;
    eng_initialize();
}
void BaseEngine::eng_initialize()
{
    m_id = 0;
#if SUPPORT_MULTIPLE_ENGINES
    static int s_engine_counter = 0;
    m_id = s_engine_counter;
    s_engine_counter++;
#endif
    if (ConsoleHelper::is_initialized() && ConsoleHelper::FindGlobal() != this->conhelper)
    {
        //intialized oddly
        conhelper = ConsoleHelper::FindGlobal();
        //
#ifdef _DEBUG
        console_printf("DBG: ConHelper was intitialized oddly\n");
#endif
    }
    else
    {
        conhelper = new ConsoleHelper(false);
}
}
#pragma warning (pop)

struct console_thread_pass
{
    BaseEngine* m_engine;
    RENDER_WINDOW_CLASS* m_window;
    EngineConsole* m_console;
};

void console_thread(console_thread_pass f_pass)
{
    while (f_pass.m_window->isOpen())
    {
        f_pass.m_console->process(f_pass.m_engine);
    }
}

class EngineSuperSecret
{
public:
    sf::Clock* clockp;
    EngineConsole console;
};



int BaseEngine::run()
{
    //these need to be first
    UnpackConBall();
    UnpackEntBall();

    OnPreInitialized();
    //
    typedef void (BaseEngine::* updateloop)();

    active = 1;
    sf::ContextSettings settings = sf::ContextSettings();
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 4;
    settings.minorVersion = 0;
	// Create the main window
	sf::Window window(sf::VideoMode(800, 600), GetTitle() , sf::Style::Default, settings);
    m_Window = &window;
    window.setVerticalSyncEnabled(true);
    //window.setFramerateLimit(15);
    if (FORBID_RENDERING)
    {
        //bug: window flashes for a very small amount of time using this
        ::ShowWindow(window.getSystemHandle(), SW_HIDE);
    }
    else
    {
#if !_DEBUG
        //hide the console
        ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif
    }
    m_supersecret = new EngineSuperSecret();
    //using function pointers here to avoid ugly if statement
    updateloop upd;

    //this is ugly.. but hey this is init so whatever
    if (FORBID_RENDERING)
    {
        upd = &BaseEngine::updateloop_txt;
    }
    else
    {
        upd = &BaseEngine::updateloop_win;

    }


    const char* enginever = engine_version.tostring();
    printf("mezmerize version %s\n", enginever);
    //print the name of the engine
    printf("%s %s\n", this->GetName(), this->GetVersion_String());
    std::cout << "build time: " << BUILD_DATE << "," << BUILD_TIME << "\n";


    engine = this;
    if (ALLOW_RENDERING)
    {
        render_setup();
    }
   
    console_thread_pass pass = { this,m_Window,&m_supersecret->console };
    sf::Thread cthread(&console_thread, pass);
    cthread.launch();
    //load cache house
    cache.load();
    sf::Clock clock; // starts the clock
    m_supersecret->clockp = &clock;
    OnInitialized();
    //THIS IS THE LOOP!!!

    while (window.isOpen())
    {
        Input::tic();
        // Process events
        sf::Event event;
        while (window.pollEvent(event))
        {
            //this may need to be changed to a switch statement later
            // Close window: exit
            if (event.type == sf::Event::Closed)
            {
                window.close();
                break;
            }
            if (event.type == sf::Event::Resized)
            {
                rendersys.m_viewport_size.x = event.size.width;
                rendersys.m_viewport_size.y = event.size.height;
                rendersys.notify_vpupdate();
                break;
            }
            if (event.type == sf::Event::KeyPressed)
            {
                Input::notify_key_pressed((MezKeyCode)event.key.scancode);
                break;
            }
            if (event.type == sf::Event::KeyReleased)
            {
                Input::notify_key_released((MezKeyCode)event.key.scancode);
                break;
            }
            if (event.type == sf::Event::GainedFocus)
            {
            Input::notify_focus(1);
                break;
            }
            if (event.type == sf::Event::LostFocus)
            {
                Input::notify_focus(0);
                break;
            }

        }
        Input::post_tic();

        engine = this;
        if (m_immediate_operation)
        {
            m_immediate_operation();
            m_immediate_operation = 0;
        }

        float old_time = time;
        time = clock.getElapsedTime().asSeconds();
        time_alt = clock.getElapsedTime().asMicroseconds();
        time_delta = time - old_time;
        //update
        (this->*upd)();

        // Update the window
        //window.display();
    }
    if (window.isOpen())
    {
        window.close();
    }

    cthread.terminate();
    active = 0;
    return EXIT_SUCCESS;
}

void BaseEngine::fatalmsg(const char* msg)
{
    //todo: use gtk to show an error message or something
#if _DEBUG
    show_messagebox(msg, ENGINE_MSGBOXTYPE_ERROR);
#endif
    abort();
}
//#1 is question which is deprecated by windows
const char* msgbox_descr[] = {"Error","","Warning","Information"};

void BaseEngine::show_messagebox(const char* f_msg, ENGINE_MSGBOXTYPE_T f_type)
{
    printf(f_msg);
    int r = MessageBoxA(m_Window->getSystemHandle(), f_msg, static_format("Engine - %s",msgbox_descr[f_type-1]), f_type << 4);
}

void BaseEngine::push_immediate_operation(voidfunction_t& function)
{
    assert(!m_immediate_operation);
    if (m_immediate_operation) { abort(); }
    m_immediate_operation = function;
}

void BaseEngine::reset_globals()
{
    this->m_supersecret->clockp->restart();
}

void BaseEngine::stop()
{
    active = 0;
    m_Window->close();
}

void BaseEngine::update()
{
    elist.update();
}

void BaseEngine::render()
{
    rendersys.Render_CallThisInEnginePlease();
}

void BaseEngine::updateloop_win()
{
    update();
    render();
}

void BaseEngine::updateloop_txt()
{
    update();
}

void BaseEngine::render_setup()
{
    rendersys.Engine_Setup(m_Window);
    rendersys.m_viewport_size = Vector2(800,600);
    rendersys.notify_vpupdate();
}

int BaseEngine::RunEngine()
{
    BaseEngine* engine = new BaseEngine();
    int c = engine->run();
    delete engine;
    return c;
}

void* BaseEngine::GetWindowHandle()
{
    return m_Window->getSystemHandle();
}

MezBaseEntity* BaseEngine::CreateEntityByName_Typeless(const char* m_name)
{
    efactory_t* f = entity_factories_t::find(m_name,this);
    if (!f) return 0;
    return f->m_func();
}

bool BaseEngine::RunCommand(const char* cmd)
{
    m_supersecret->console.process_single(this,cmd);
    return false;
}

int BaseEngine::GetVersion_Numeric()
{
    return MEZ_VERSION_NUMERIC;
}

EngineConsole* BaseEngine::GetEngineConsole()
{
    return &this->m_supersecret->console;
}

ConBall BaseEngine::GetConBall()
{
    //return ConBall();
    return get_stock_conball();
}

EntBall BaseEngine::GetEntBall()
{
    //return EntBall();
    return get_stock_entball();
}

void BaseEngine::UnpackConBall()
{
    GetConBall().unpack();
}

void BaseEngine::UnpackEntBall()
{
    GetEntBall().unpack();
}

RInterface* BaseEngine::create_rinterface_instance()
{
    return (ALLOW_RENDERING) ? new RInterface_Default() : new RInterface_Null();
}

RStatic* BaseEngine::create_rstatic()
{
    return (ALLOW_RENDERING) ? new RStatic_Default() : new RStatic_Null();
}

void BaseEngine::set_mousepos(Vector2 f_pos)
{
    sf::Mouse::setPosition(sf::Vector2i(f_pos.x, f_pos.y),*m_Window);
}

void BaseEngine::cursorlock_status(bool* setval)
{
    if (setval)
    {
        m_Window->setMouseCursorGrabbed(*setval);
        return;
    }
}

#pragma warning (disable: 4172)
const char* EngineVersion::tostring()
{
    return MEZ_VERSION;
}

int EngineVersion::tonumeric()
{
    return MEZ_VERSION_NUMERIC;
}
