#include "fixed_point_arithmetic.h"

int f(void){
	return 1<<14;
}

int integer_to_fixedpoint(int n){
	return n * f();
}

int fixedpoint_to_integer_towardzero(int x){
	return x / f();
}

int fixedpoint_to_integer_nearest(int x){
	return x >= 0 ? fixedpoint_to_integer_towardzero(x + f() / 2): fixedpoint_to_integer_towardzero(x - f() / 2);
}

int add_fixedpoints(int x,int y){
	return x + y;
}

int substract_fixedpoints(int x, int y){
	return x - y;
}

int add_fixedpoint_integer(int x,int n){
	return add_fixedpoints(x,integer_to_fixedpoint(n));
}
int substract_fixedpoint_integer(int x,int n){
	return substract_fixedpoints(x,integer_to_fixedpoint(n));
}

int multiply_fixedpoints(int x,int y){
	return ((int64_t) x) * y / f();
}

int multiply_fixedpoint_integer(int x,int n){
	return x * n;
}

int divide_fixedpoints(int x,int y){
	return ((int64_t) x) * f() / y;
}

int divide_fixedpoint_integer(int x,int n){
	return x / n;
}