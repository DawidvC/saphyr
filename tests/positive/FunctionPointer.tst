
int func(int a)
{
	return a + 1;
}

int func2()
{
	@(int)int a = func;
	@(int)int b = null;

	b = func$;

	return a(5) + b(4);
}

int main()
{
	auto ptr = func;

	ptr(4);

	auto ptr2 = func$$;

	ptr2(4);

	return 0;
}

========

define i32 @func(i32 %a) {
  %1 = alloca i32
  store i32 %a, i32* %1
  %2 = load i32* %1
  %3 = add i32 %2, 1
  ret i32 %3
}

define i32 @func2() {
  %a = alloca i32 (i32)*
  store i32 (i32)* @func, i32 (i32)** %a
  %b = alloca i32 (i32)*
  store i32 (i32)* null, i32 (i32)** %b
  store i32 (i32)* @func, i32 (i32)** %b
  %1 = load i32 (i32)** %a
  %2 = call i32 %1(i32 5)
  %3 = load i32 (i32)** %b
  %4 = call i32 %3(i32 4)
  %5 = add i32 %2, %4
  ret i32 %5
}

define i32 @main() {
  %ptr = alloca i32 (i32)*
  store i32 (i32)* @func, i32 (i32)** %ptr
  %1 = load i32 (i32)** %ptr
  %2 = call i32 %1(i32 4)
  %ptr2 = alloca i32 (i32)*
  store i32 (i32)* @func, i32 (i32)** %ptr2
  %3 = load i32 (i32)** %ptr2
  %4 = call i32 %3(i32 4)
  ret i32 0
}
