#ifndef _fixed_point_arithmetic_
#define _fixed_point_arithmetic_
//x-fixedpoint y-fixedpoint n-integer
int f(); //2^
int integer_to_fixedpoint(int n); //convert integer to fixedpoint
int fixedpoint_to_integer_towardzero(int x); //convert fixedpoint to integer(rounding toward zero)
int fixedpoint_to_integer_nearest(int x); // conver fixedpoint to integer(rounding to nearest)
int add_fixedpoints(int x,int y); //add x and y
int substract_fixedpoints(int x,int y); //substract y from x
int add_fixedpoint_integer(int x,int n);//add x and n
int substract_fixedpoint_integer(int x,int n); //substract n from x
int multiply_fixedpoints(int x,int y); //multiply x by y
int multiply_fixedpoint_integer(int x,int n); //multiply x by n
int divide_fixedpoints(int x,int y); //divide x by y
int divide_fixedpoint_integer(int x,int n);//divide x by n

#endif