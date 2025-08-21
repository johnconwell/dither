#include <iostream>
#include <vector>
#include "dither.h"
#include "grayscale.h"



int main()
{
    Dither d = Dither();
    d.load("input\\mona_lisa.png");
    d.grayscale(GrayscaleMethod::BT709);
    d.save("output\\mona_lisa_grayscale.png");

    // Color c = Color(155, 216, 17, 255);
    // uint8_t value = Grayscale::channel_value(c, GrayscaleMethod::STANDARD);
    // printf("%i\n", value);
    // // std::cout << Grayscale::channel_value(c, GrayscaleMethod::BT709) + '0' << std::endl;

    std::cout << "finished" << std::endl;
    return 0;
}