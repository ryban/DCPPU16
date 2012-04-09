#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include "dcpu.h"

bool valid_number_string(char *str)
{
    for(int c = 0; str[c]; c++)
        if(!isdigit(str[c]))
            return false;

    return true;
}
void start(void *usr)
{
    // CPU thread
    Dcpu *cpu = static_cast<Dcpu *> (usr);
    cpu->run();
}

char ShiftChar(char c)
{
    if(isalpha(c))
        return toupper(c);

    // all if this does not work...
    switch(c)
    {
        case '1': return '!';
        case '2': return '@';
        case '3': return '#';
        case '4': return '$';
        case '5': return '%';
        case '6': return '^';
        case '7': return '&';
        case '8': return '*';
        case '9': return '(';
        case '0': return ')';
        case '-': return '_';
        case '=': return '+';
        case '`': return '~';
        case '[': return '{';
        case ']': return '}';
        case ';': return ':';
        case '\'': return '"';
        case ',': return '<';
        case '.': return '>';
        case '/': return '?';
        case '\\': return '|';
        default:
            return c;
    }
}
void AbnormalChar(sf::Event &Event, char &c)
{
    // SFML doesn't map directly to ascii
    // so I got this...

    if(Event.Key.Code == sf::Key::Return)
        c = '\n';
    else if(Event.Key.Code == sf::Key::Space)
        c = ' ';
    else if(Event.Key.Code == sf::Key::BackSlash)
        c = '\\';
    else if(Event.Key.Code == sf::Key::LBracket)
        c = '[';
    else if(Event.Key.Code == sf::Key::RBracket)
        c = ']';
    else if(Event.Key.Code == sf::Key::SemiColon)
        c = ';';
    else if(Event.Key.Code == sf::Key::Comma)
        c = ',';
    else if(Event.Key.Code == sf::Key::Period)
        c = '.';
    else if(Event.Key.Code == sf::Key::Quote)
        c = '"';
    else if(Event.Key.Code == sf::Key::Slash)
        c = '/';
    else if(Event.Key.Code == sf::Key::Tilde)
        c = '`';
    else if(Event.Key.Code == sf::Key::Equal)
        c = '=';
    else if(Event.Key.Code == sf::Key::Dash)
        c = '-';
    else if(Event.Key.Code == sf::Key::Tab)
        c = '\t';
    else if(Event.Key.Code == sf::Key::Delete)
        c = 127;
    else if(Event.Key.Code == sf::Key::Back)
        c = 8;
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        cerr << "usage: dcpu <flags> <filename>\n";
        return 1;
    }

    bool debug = false;
    bool fast = false;
    int time_to_kill_ms = 0;

    // -d runs in debug mode, prints out memory dump at end
    // -t <value> is the time in ms until the program exits (not very useful anymore)
    // -f will not perform cycle checks, i.e. ignore the wait(cycles) command (Does nothing at the moment)

    for(int a = 1; a < argc - 1; a++)
    {
        if(!strcmp(argv[a], "-d"))
        {
            debug = true;
        }else if(!strcmp(argv[a], "-f"))
        {
            fast = true;
        }else
        {
            cerr << "invalid command <" << argv[a] << ">. ignoring\n";
        }
    }

    ifstream inFile(argv[argc - 1], ios::in);
    if(!inFile.good())
    {
        cerr << "Could not open <" << argv[argc - 1] << ">\n";
        return 1;
    }

    Dcpu cpu(inFile, debug, fast);

    sf::Thread cpu_thread(&start, &cpu);

    cpu_thread.Launch();

    sf::RenderWindow app(sf::VideoMode(TERMINAL_WIDTH * 18 + 32, TERMINAL_HEIGHT * 32 + 5), "DCPPU: DCPU-16 Emulator");
    bool running = true;

    app.SetFramerateLimit(30); // Getting 1500fps on somthing this simple seems wasteful

    while(running)
    {
        sf::Event Event;
        while(app.GetEvent(Event))
        {
            if(Event.Type == sf::Event::Closed)
            {
                cpu.kill();
                running = false;
                app.Close();
            }
            if(Event.Type == sf::Event::KeyPressed)
            {
                char c = Event.Key.Code;
                AbnormalChar(Event, c);
                if(Event.Key.Shift)
                    cpu.PushInBuff(ShiftChar(c));
                else
                    cpu.PushInBuff(c);

            }

        }
        app.Clear(sf::Color(56, 83, 255));

        sf::String Text;
        Text.SetSize(32);

        unsigned short *buffer = cpu.GetScreenBuffer();

        int row = 0;
        int col = 0;

        char *row_t = new char[TERMINAL_WIDTH+1];
        row_t[TERMINAL_WIDTH] = 0;

        for(int i = 0; i < TERMINAL_HEIGHT * TERMINAL_WIDTH; i++)
        {
            char c = buffer[i];
            if(c == '\n' || col >= TERMINAL_WIDTH)
            {
                Text.SetText(row_t);
                Text.SetPosition(15, row * 32 + 5);
                app.Draw(Text);

                col = 0;
                row++;
                if(row >= TERMINAL_HEIGHT)
                    break;
                continue;
            }
            if(c == 0 || c == '\t')
            {
                c = ' ';
            }
            row_t[col] = c;
            col++;
        }

        app.Display();
    }
    
    return 0;
}