#include <omen/libraries/std/math.h>

double pow(double base, double exponent) {
    double result = 1;
    for (int i = 0; i < exponent; i++) {
        result *= base;
    }
    return result;
}
double sqrt(double x) {
    double result = 1;
    for (int i = 0; i < 10; i++) {
        result = 0.5 * (result + x / result);
    }
    return result;

}
double cbrt(double x) {
    double result = 1;
    for (int i = 0; i < 10; i++) {
        result = (1.0 / 3.0) * ((2 * result) + (x / (result * result)));
    }
    return result;

}
double hypot(double x, double y) {
    return sqrt(pow(x, 2) + pow(y, 2));
}
double exp(double x) {
    double result = 1;
    for (int i = 0; i < 10; i++) {
        result += pow(x, i) / factorial(i);
    }
    return result;
}
double factorial(double x) {
    double result = 1;
    for (int i = 1; i <= x; i++) {
        result *= i;
    }
    return result;
}