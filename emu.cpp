#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <ctime>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include "dcpu.h"

sf::Color color_table[16] = {
    sf::Color::Black,
    sf::Color(128, 0, 0),       // maroon
    sf::Color(0, 128, 0),       // dark green
    sf::Color(128, 128, 0),     // olive
    sf::Color(0, 0, 128),       // navy blue
    sf::Color(128, 0, 128),     // purple
    sf::Color(0, 128, 128),     // teal/cyan
    sf::Color(192, 192, 192),   // silver
    sf::Color(128, 128, 128),   // grey
    sf::Color::Red,
    sf::Color::Green,
    sf::Color::Yellow,
    sf::Color::Blue,
    sf::Color::Magenta,
    sf::Color::Cyan,
    sf::Color::White
};

string itoa(long i)
{
    std::string s;
    std::stringstream out;
    out << i;
    return out.str();
}

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
    else if(Event.Key.Code == sf::Key::Escape)
        c = 27;
}

void DrawCharacter(sf::Image &screen, unsigned short *font_buff, char c, sf::Color fg_col, sf::Color bg_col, bool blink, int x, int y)
{
    /*
    set the area on the screen to the proper character c, defined in the font_buf
    use the colors fg_col and bg_col
    fonts defined in 2 words
    upperword aaaa bbbb cccc dddd
    lowerword eeee ffff gggg hhhh
    character defined as
    
        a a a a
        b b b b
        c c c c
        d d d d
        e e e e
        f f f f
        g g g g
        h h h h
    
    1 means set to fg_col, 0 means set to bg col
    */
    int font_off_x = x * 4;
    int font_off_y = y * 8;
    //font_buff += (c * 2);   // 1 char per word

    unsigned short fontupperword = font_buff[(c * 2)];
    unsigned short fontlowerword = font_buff[(c * 2) + 1];

    unsigned int font_char = fontupperword << 16 | fontlowerword;

    //for(int off = 0; off < 32; off++) // for reverse
    for(int off = 31; off >= 0; off--)
    {
        unsigned int mask = font_char & (1 << (31-off));
        int x_img = font_off_x + (off % 4);
        int y_img = font_off_y + (off / 4);

        if(mask > 0 && !blink)
            screen.SetPixel(x_img, y_img, fg_col);
        else
            screen.SetPixel(x_img, y_img, bg_col);
        
    }
}

void reverse_bits(unsigned int &v)
{
    // code from : http://graphics.stanford.edu/~seander/bithacks.html#BitReverseObvious
    unsigned int r = v; // r will be reversed bits of v; first get LSB of v
    int s = sizeof(v) * CHAR_BIT - 1; // extra shift needed at end

    for (v >>= 1; v; v >>= 1)
    {   
      r <<= 1;
      r |= v & 1;
      s--;
    }
    r <<= s; // shift when v's highest bits are zero
    v = r;
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        cerr << "usage: dcpu <flags> <filename>\n";
        return 1;
    }

    bool debug = false;
    int time_to_kill_ms = 0;

    // -d runs in debug mode, prints out memory dump at end

    for(int a = 1; a < argc - 1; a++)
    {
        if(!strcmp(argv[a], "-d"))
        {
            debug = true;
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

    Dcpu cpu(inFile, debug);

    sf::Thread cpu_thread(&start, &cpu);

    sf::RenderWindow app(sf::VideoMode(TERMINAL_WIDTH * 4 * 4, TERMINAL_HEIGHT * 8 * 4), "DCPPU: DCPU-16 Emulator");

    app.SetFramerateLimit(30); // Getting 1500 fps on somthing this simple seems wasteful

    sf::Image font;
    if(!font.LoadFromFile("font.png"))
    {
        cerr << "Could not load font\n";
        return 1;
    }

    unsigned short *buf = cpu.GetScreenBuffer();
    buf += TERMINAL_WIDTH * TERMINAL_HEIGHT;

    // load and encode character set into RAM
    for(int char_off = 0; char_off < 128; char_off++)
    {
        int font_off_x = (char_off * 4) % 128;
        int font_off_y = ((char_off * 4) / 128) * 8;

        unsigned int font_char = 0;
        int x_ = 0;
        int y_ = 0;
        for(int x = font_off_x; x < font_off_x + 4; x++)
        {
            for(int y = font_off_y; y < font_off_y + 8; y++)
            {
                sf::Color pix = font.GetPixel(x, y); 
                if(pix != sf::Color(2, 1, 2))
                    font_char |= 1 << (31-((y_ * 4) + x_));
                y_++;
            }
            x_++;
        }
        unsigned short lowerword = font_char & 0xffff;
        unsigned short upperword = (font_char >> 16) & 0xffff;
        buf[char_off * 2] = upperword;
        buf[char_off * 2 + 1] = lowerword;
    }


    sf::Image screen;
    screen.Create(128, 96); // create black image
    screen.SetSmooth(false);
    sf::Sprite screen_sprite(screen);
    screen_sprite.SetScale(4.0, 4.0);

    cpu_thread.Launch();

    bool running = true;
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
                AbnormalChar(Event, c);     // fix incorrect ascii values sfml uses
                if(Event.Key.Shift)
                    cpu.PushInBuff(ShiftChar(c));
                else
                    cpu.PushInBuff(c);
            }
        }
        app.Clear();

        unsigned short *buffer = cpu.GetScreenBuffer();

        for(int y = 0; y < TERMINAL_HEIGHT; y++)
        {
            for(int x = 0; x < TERMINAL_WIDTH; x++)
            {
                unsigned short sc = buffer[y * 32 + x];
                int fg_off = (sc >> 12) & 0xf;
                int bg_off = (sc >> 8) & 0xf;
                sf::Color fg = color_table[fg_off];
                sf::Color bg = color_table[bg_off];
                char c = sc & 0x7f;
                bool blink = (sc & 0x80) == 0x80 && (time(0) % 2 == 0); // if bit 15 set, blink on for 1 sec, off for 1 sec

                DrawCharacter(screen, buffer + (TERMINAL_WIDTH*TERMINAL_HEIGHT), c, fg, bg, blink, x, y);
            }
        }

        app.Draw(screen_sprite);
        app.Display();
    }
    
    return 0;
}