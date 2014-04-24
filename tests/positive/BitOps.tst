
int func()
{
	int a, b, c;

	a = b & c;
	b = c | a;
	c = a ^ b;

	return a | b ^ c;
}

========

define i32 @func() {
  %a = alloca i32
  %b = alloca i32
  %c = alloca i32
  %1 = load i32* %b
  %2 = load i32* %c
  %3 = and i32 %1, %2
  store i32 %3, i32* %a
  %4 = load i32* %c
  %5 = load i32* %a
  %6 = or i32 %4, %5
  store i32 %6, i32* %b
  %7 = load i32* %a
  %8 = load i32* %b
  %9 = xor i32 %7, %8
  store i32 %9, i32* %c
  %10 = load i32* %a
  %11 = load i32* %b
  %12 = load i32* %c
  %13 = xor i32 %11, %12
  %14 = or i32 %10, %13
  ret i32 %14
}
