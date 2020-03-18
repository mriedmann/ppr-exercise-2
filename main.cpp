#include <cstdio>
#include <cstdlib>
#include <omp.h>
#include "tga.h"
#include <tuple>
#include <math.h>
#include <vector>

typedef struct RgbColor
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
} RgbColor;

typedef struct HsvColor
{
    unsigned char h;
    unsigned char s;
    unsigned char v;
} HsvColor;

RgbColor* image;

double minx = -2.0;
double miny = -1.5;
double maxx = 1.0;
double maxy = 1.5;
int width = 1024;
int height = 1024;

int maxIterations = 1000;

RgbColor HsvToRgb(HsvColor hsv)
{
    RgbColor rgb;
    unsigned char region, remainder, p, q, t;

    if (hsv.s == 0)
    {
        rgb.r = hsv.v;
        rgb.g = hsv.v;
        rgb.b = hsv.v;
        return rgb;
    }

    region = hsv.h / 43;
    remainder = (hsv.h - (region * 43)) * 6;

    p = (hsv.v * (255 - hsv.s)) >> 8;
    q = (hsv.v * (255 - ((hsv.s * remainder) >> 8))) >> 8;
    t = (hsv.v * (255 - ((hsv.s * (255 - remainder)) >> 8))) >> 8;

    switch (region)
    {
        case 0:
            rgb.r = hsv.v; rgb.g = t; rgb.b = p;
            break;
        case 1:
            rgb.r = q; rgb.g = hsv.v; rgb.b = p;
            break;
        case 2:
            rgb.r = p; rgb.g = hsv.v; rgb.b = t;
            break;
        case 3:
            rgb.r = p; rgb.g = q; rgb.b = hsv.v;
            break;
        case 4:
            rgb.r = t; rgb.g = p; rgb.b = hsv.v;
            break;
        default:
            rgb.r = hsv.v; rgb.g = p; rgb.b = q;
            break;
    }

    return rgb;
}

std::tuple<double, double> normalizeToViewRectangle(int pX, int pY, double minX, double minY, double maxX, double maxY) {
    double cX = pX * ((maxX - minX) / width) + minX;
    double cY = pY * ((maxY - minY) / height) + minY;

    return {cX, cY};
}

void calcPix (int px, int py) {
    auto [cx ,cy] = normalizeToViewRectangle(px ,py ,minx ,miny ,maxx ,maxy);
    double zx = cx;
    double zy = cy;

    for (int n = 0; n < maxIterations; n++) {
        double x = (zx * zx - zy * zy ) + cx;
        double y = (zy * zx + zx * zy ) + cy;
        if (( x * x + y * y ) > 4) {
            // diverge , produce nice color
            auto h = (unsigned char)(log(1.0 + n) / log(1.0 + maxIterations) * 255);
            HsvColor hsvColor = { h, 255, 255 };
            RgbColor rgbColor = HsvToRgb(hsvColor);
            image[px * width + py] = rgbColor;
            return;
        }
        zx = x;
        zy = y;
    }
    // no diverge, print black
    image[px * width + py] = {0,0,0};
}

int main (int argc, char *argv[]) {
    image = new RgbColor[width * height];

    double startTime = omp_get_wtime();
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            calcPix(x, y);
        }
    }
    double endTime = omp_get_wtime();
    double Ts = endTime - startTime;
    printf("Serial Calc Time: %f\n", Ts);

    startTime = omp_get_wtime();
#pragma omp parallel for collapse(2) shared(width,height) default(none) schedule(static,16)
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            calcPix(x, y);
        }
    }
    endTime = omp_get_wtime();
    double Tn = endTime - startTime;
    printf("Parallel Calc Time: %f\n", Tn);
    printf("Ts/Tn: %f\n", Ts/Tn);

    startTime = omp_get_wtime();
    tga::TGAImage tgaImage;
    tgaImage.height = height;
    tgaImage.width = width;
    tgaImage.bpp = 24;
    tgaImage.type = 0;

    for(int i = 0; i < width * height; i++) {
        RgbColor rgbColor = image[i];

        tgaImage.imageData.push_back(rgbColor.r);
        tgaImage.imageData.push_back(rgbColor.g);
        tgaImage.imageData.push_back(rgbColor.b);
    }
    endTime = omp_get_wtime();
    printf("Draw Time: %f\n", endTime - startTime);

    startTime = omp_get_wtime();
    tga::saveTGA(tgaImage, "output.tga");
    endTime = omp_get_wtime();
    printf("Write Time: %f\n", endTime - startTime);

    return 0;
}
