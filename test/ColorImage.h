#ifndef TEST_COLOR_IMAGE_H
#define TEST_COLOR_IMAGE_H

#include <CImg.h>
#include <dStorm/engine/Image.h>
#include <dStorm/engine/Spot.h>
#include <dStorm/engine/Localization.h>

class ColorImage : public cimg_library::CImg<dStorm::StormPixel> {
   private:
      int min_grey_value, max_grey_value;
      dStorm::StormPixel colours[8][3];
      int xo, yo;

   public:
      ColorImage (const dStorm::Image&, 
         int xlow = -1, int xhigh = -1, int ylow = -1, int yhigh = -1)
         throw();

      enum Colors { Black, Red, Green, Yellow, Blue, Magenta, Cyan, White };

      void mark(const dStorm::Spot &s, int colour = 1) throw();
      void mark(const dStorm::Localization &s, int colour = 1) throw() {
         mark(dStorm::Spot(s.x(), s.y()), 
                  /*(s.isGood()) ?*/ colour /*: (White - colour)*/);
      }
};

#endif
