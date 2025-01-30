#include <emmintrin.h>
#include <smmintrin.h>
#include <stdio.h>
#include <string.h>

//all SIMD
#include <immintrin.h>
#include <xmmintrin.h>

#define NUM_THREADS 1
const int WIDTH = 8;
const int HEIGHT = 8;

typedef struct {
    size_t size;
    char* data;
    int offset;
    double cx, cy;
    double zoom;
} thread_data;

void* render_thread(void* argp) {
    thread_data* d = argp;

    size_t size = d->size;
    char* data = d->data;
    double cx = d->cx;
    double cy = d->cy;
    double zoom = d->zoom;
    int offset = d->offset;
    for (int i = offset; i < size/3; i += NUM_THREADS) {
        double x = (double)((i) % WIDTH) / WIDTH;
        double y = (((double)(i)) / HEIGHT) / HEIGHT;

        x -= 0.5;
        y -= 0.5;
        x *= zoom;
        y *= zoom;
        x -= cx;
        y -= cy;

        double px, py;
        float color = 255;
        for (int j = 0; j < 100; j++) {
            px = x;
            py = y;

            x = px * px - py * py;
            y = 2 * px * py;
            x -= 1;
            if (x*x + y*y >= 4) {
                color = 0; 
                break;
            }
        }

        //SR_LOG_DEB("c: %f %f", __real__ c, __imag__ c);
        data[3*i] = color; 
        data[3*i+1] = color;
        data[3*i+2] = color;



    }
    return NULL;
}

void* render_thread_SIMD_julia(void* argp) {
    thread_data* d = argp;

    size_t size = d->size;
    char* data = d->data;
    double cx = d->cx;
    double cy = d->cy;
    double zoom = d->zoom;
    int offset = d->offset;
    //four wide
    
    __m256d coordscale = _mm256_set_pd(WIDTH, WIDTH, WIDTH, WIDTH);
    __m256d centerx = _mm256_set_pd(cx, cx, cx, cx);
    __m256d centery = _mm256_set_pd(cy, cy, cy, cy);

    __m256d mult2 = _mm256_set_pd(2, 2, 2, 2);
    __m256d one = _mm256_set1_pd(1);

    __m256d zoomscale = _mm256_set_pd(zoom, zoom, zoom, zoom);
    __m256d offset_c = _mm256_set_pd(0.5, 0.5, 0.5, 0.5);

    __m256d threshhold = _mm256_set_pd(4.0, 4.0, 4.0, 4.0);

    __m256d constant = _mm256_set_pd(1.0, 1.0, 1.0, 1.0);

    for (int i = offset; i < size/3; i += NUM_THREADS * 4) {
        __m256d indicies = _mm256_set_pd(i + 3, i + 2, i + 1, i + 0);

        //get components
        indicies = _mm256_div_pd(indicies, coordscale);
        __m256d yvals = _mm256_round_pd(indicies, _MM_FROUND_TRUNC);
        __m256d xvals = _mm256_sub_pd(indicies, yvals); 
        yvals = _mm256_div_pd(yvals, coordscale);


        xvals = _mm256_sub_pd(xvals, offset_c);
        yvals = _mm256_sub_pd(yvals, offset_c);


        xvals = _mm256_mul_pd(xvals, zoomscale);
        yvals = _mm256_mul_pd(yvals, zoomscale);

        xvals = _mm256_sub_pd(xvals, centerx);
        yvals = _mm256_sub_pd(yvals, centery);

        //`double debug[4];
        //`_mm256_storeu_pd(debug, xvals);
        //`printf("%f %f %f %f\n", debug[0], debug[1], debug[2], debug[3]);

        //`_mm256_storeu_pd(debug, yvals);
        //`printf("%f %f %f %f\n\n", debug[0], debug[1], debug[2], debug[3]);
        //iter

        __m256d valid = _mm256_setzero_pd();

        __m256d iters = _mm256_set1_pd(0);
        for (int j = 0; j < 100; j++) {
            __m256d x2 = _mm256_mul_pd(xvals, xvals);
            __m256d y2 = _mm256_mul_pd(yvals, yvals);
            __m256d xy = _mm256_mul_pd(xvals, yvals);

            xvals = _mm256_sub_pd(_mm256_sub_pd(x2, y2), one);
            yvals = _mm256_add_pd(xy, xy);

            __m256d cmp = _mm256_cmp_pd(_mm256_add_pd(x2,y2), threshhold, _CMP_LT_OS);
            iters = _mm256_add_pd(_mm256_and_pd(cmp, one), iters);

            if (_mm256_testz_pd(cmp, _mm256_set1_pd(-1))) {
                break;
            }
        }

        double results[4];

        __m256d tx = _mm256_mul_pd(xvals, xvals);
        __m256d ty = _mm256_mul_pd(yvals, yvals);

        //_mm256_storeu_pd(debug, xvals);
        //printf("%f %f %f %f\n", debug[0], debug[1], debug[2], debug[3]);

        //_mm256_storeu_pd(debug, yvals);
        //printf("%f %f %f %f\n\n", debug[0], debug[1], debug[2], debug[3]);

        _mm256_storeu_pd(results, iters);


        for (int j = 0; j < 4; j++) {
            printf("%f ", results[j]);
            if (results[j] >= 100) {
                data[(i+j)*3 + 0] = (char)255;
                data[(i+j)*3 + 1] = (char)255;
                data[(i+j)*3 + 2] = (char)255;
            } else {
                data[(i+j)*3 + 0] = (char)0;
                data[(i+j)*3 + 1] = (char)0;
                data[(i+j)*3 + 2] = (char)0;
            }
        }
        printf("\n");

    }
    return NULL;
}


int main() {
    printf("Start\n");


    char buffer[WIDTH*HEIGHT*3];
    thread_data d = (thread_data){
        .cx = 0,
        .cy = 0,
        .data = buffer,
        .size = sizeof(buffer),
        .zoom = 2,
        .offset = 2
    };

    //non-SIMD
    render_thread(&d);

    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            int index = j*3 + i*3*WIDTH;
            printf("%.1d %.1d %.1d | ", buffer[index], buffer[index + 1], buffer[index+2]);
        }
        printf("\n");
    }
    printf("\n");

    //SIMD
    char buffer2[WIDTH*HEIGHT*3];
    memset(buffer2, 0, sizeof(buffer2));
    d.data = buffer2;
    render_thread_SIMD_julia(&d);

    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            int index = j*3 + i*3*WIDTH;
            printf("%.1d %.1d %.1d | ", buffer2[index], buffer2[index + 1], buffer2[index+2]);
        }
        printf("\n");
    }
    printf("\n");


    //diff
    
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            int index = j*3 + i*3*WIDTH;
            printf("%.1d %.1d %.1d | ", buffer[index] - buffer2[index],
                    buffer[index+1] - buffer2[index + 1],
                    buffer[index+2] - buffer2[index+2]);
        }
        printf("\n");
    }


    return 0;
}
