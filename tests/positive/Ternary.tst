
struct S
{
	int a, b;
}

int four()
{
	S z;
	return 4? 5 : z.a;
}

int func()
{
	int x = 4;
	return x > 3? func() : four(); 
}

int main()
{
	int x = 5;

	auto f = x < 10? 1.0 : 3.5;
	auto z = f > 10? 4 + f : x + 1.0;

	return z;
}

========

%S = type { i32, i32 }

define i32 @four() {
  %z = alloca %S
  %1 = icmp ne i32 4, 0
  br i1 %1, label %5, label %2

; <label>:2                                       ; preds = %0
  %3 = getelementptr %S* %z, i32 0, i32 0
  %4 = load i32* %3
  br label %5

; <label>:5                                       ; preds = %0, %2
  %6 = phi i32 [ %4, %2 ], [ 5, %0 ]
  ret i32 %6
}

define i32 @func() {
  %x = alloca i32
  store i32 4, i32* %x
  %1 = load i32* %x
  %2 = icmp sgt i32 %1, 3
  br i1 %2, label %3, label %5

; <label>:3                                       ; preds = %0
  %4 = call i32 @func()
  br label %7

; <label>:5                                       ; preds = %0
  %6 = call i32 @four()
  br label %7

; <label>:7                                       ; preds = %5, %3
  %8 = phi i32 [ %4, %3 ], [ %6, %5 ]
  ret i32 %8
}

define i32 @main() {
  %x = alloca i32
  store i32 5, i32* %x
  %1 = load i32* %x
  %2 = icmp slt i32 %1, 10
  %3 = select i1 %2, double 1.000000e+00, double 3.500000e+00
  %f = alloca double
  store double %3, double* %f
  %4 = load double* %f
  %5 = sitofp i32 10 to double
  %6 = fcmp ogt double %4, %5
  br i1 %6, label %7, label %11

; <label>:7                                       ; preds = %0
  %8 = load double* %f
  %9 = sitofp i32 4 to double
  %10 = fadd double %9, %8
  br label %15

; <label>:11                                      ; preds = %0
  %12 = load i32* %x
  %13 = sitofp i32 %12 to double
  %14 = fadd double %13, 1.000000e+00
  br label %15

; <label>:15                                      ; preds = %11, %7
  %16 = phi double [ %10, %7 ], [ %14, %11 ]
  %z = alloca double
  store double %16, double* %z
  %17 = load double* %z
  %18 = fptosi double %17 to i32
  ret i32 %18
}
