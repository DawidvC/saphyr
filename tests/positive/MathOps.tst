
void mult()
{
	float f;
	int i;
	uint u;

	f = f * i;
	f = i * u;
	u = f * u;
}

void div()
{
	double f, d;
	int i;
	uint u;

	i = f / u;
	f = (i + 1) / i;
	i = u / i;
}

void mod()
{
	double f, d;
	int i;
	uint u;

	i = f % i;
	f = (i + 1) % i;
	i = u % i;
}

void add()
{
	double f, d;
	int i;
	uint u;

	i = f + d;
	f = i + i;
	i = u + i;
}

void sub()
{
	double f, d;
	int i;
	uint u;

	u = f - d;
	f = i - i;
	i = u - i;
}

========

define void @mult() {
  %f = alloca float
  %i = alloca i32
  %u = alloca i32
  %1 = load float* %f
  %2 = load i32* %i
  %3 = sitofp i32 %2 to float
  %4 = fmul float %1, %3
  store float %4, float* %f
  %5 = load i32* %i
  %6 = load i32* %u
  %7 = mul i32 %5, %6
  %8 = uitofp i32 %7 to float
  store float %8, float* %f
  %9 = load float* %f
  %10 = load i32* %u
  %11 = uitofp i32 %10 to float
  %12 = fmul float %9, %11
  %13 = fptoui float %12 to i32
  store i32 %13, i32* %u
  ret void
}

define void @div() {
  %f = alloca double
  %d = alloca double
  %i = alloca i32
  %u = alloca i32
  %1 = load double* %f
  %2 = load i32* %u
  %3 = uitofp i32 %2 to double
  %4 = fdiv double %1, %3
  %5 = fptosi double %4 to i32
  store i32 %5, i32* %i
  %6 = load i32* %i
  %7 = add i32 %6, 1
  %8 = load i32* %i
  %9 = sdiv i32 %7, %8
  %10 = sitofp i32 %9 to double
  store double %10, double* %f
  %11 = load i32* %u
  %12 = load i32* %i
  %13 = udiv i32 %11, %12
  store i32 %13, i32* %i
  ret void
}

define void @mod() {
  %f = alloca double
  %d = alloca double
  %i = alloca i32
  %u = alloca i32
  %1 = load double* %f
  %2 = load i32* %i
  %3 = sitofp i32 %2 to double
  %4 = frem double %1, %3
  %5 = fptosi double %4 to i32
  store i32 %5, i32* %i
  %6 = load i32* %i
  %7 = add i32 %6, 1
  %8 = load i32* %i
  %9 = srem i32 %7, %8
  %10 = sitofp i32 %9 to double
  store double %10, double* %f
  %11 = load i32* %u
  %12 = load i32* %i
  %13 = urem i32 %11, %12
  store i32 %13, i32* %i
  ret void
}

define void @add() {
  %f = alloca double
  %d = alloca double
  %i = alloca i32
  %u = alloca i32
  %1 = load double* %f
  %2 = load double* %d
  %3 = fadd double %1, %2
  %4 = fptosi double %3 to i32
  store i32 %4, i32* %i
  %5 = load i32* %i
  %6 = load i32* %i
  %7 = add i32 %5, %6
  %8 = sitofp i32 %7 to double
  store double %8, double* %f
  %9 = load i32* %u
  %10 = load i32* %i
  %11 = add i32 %9, %10
  store i32 %11, i32* %i
  ret void
}

define void @sub() {
  %f = alloca double
  %d = alloca double
  %i = alloca i32
  %u = alloca i32
  %1 = load double* %f
  %2 = load double* %d
  %3 = fsub double %1, %2
  %4 = fptoui double %3 to i32
  store i32 %4, i32* %u
  %5 = load i32* %i
  %6 = load i32* %i
  %7 = sub i32 %5, %6
  %8 = sitofp i32 %7 to double
  store double %8, double* %f
  %9 = load i32* %u
  %10 = load i32* %i
  %11 = sub i32 %9, %10
  store i32 %11, i32* %i
  ret void
}
