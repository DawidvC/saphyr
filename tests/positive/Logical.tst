
int main()
{
	auto x = 5, y = 9.0;

	auto a = x && y;
	auto b = x || y;

	return a;
}

========

define i32 @main() {
  %x = alloca i32
  store i32 5, i32* %x
  %y = alloca double
  store double 9.000000e+00, double* %y
  %1 = load i32* %x
  %2 = icmp ne i32 %1, 0
  br i1 %2, label %3, label %6

; <label>:3                                       ; preds = %0
  %4 = load double* %y
  %5 = fcmp one double %4, 0.000000e+00
  br label %6

; <label>:6                                       ; preds = %3, %0
  %7 = phi i1 [ %2, %0 ], [ %5, %3 ]
  %a = alloca i1
  store i1 %7, i1* %a
  %8 = load i32* %x
  %9 = icmp ne i32 %8, 0
  br i1 %9, label %13, label %10

; <label>:10                                      ; preds = %6
  %11 = load double* %y
  %12 = fcmp one double %11, 0.000000e+00
  br label %13

; <label>:13                                      ; preds = %10, %6
  %14 = phi i1 [ %9, %6 ], [ %12, %10 ]
  %b = alloca i1
  store i1 %14, i1* %b
  %15 = load i1* %a
  %16 = zext i1 %15 to i32
  ret i32 %16
}
