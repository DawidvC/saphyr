
int main()
{
	int a, b, c;

	a = +b;
	b = -c;
	c = !a;
	b = ~c;

	return 0;
}

========

define i32 @main() {
  %a = alloca i32
  %b = alloca i32
  %c = alloca i32
  %1 = load i32* %b
  %2 = add i32 0, %1
  store i32 %2, i32* %a
  %3 = load i32* %c
  %4 = sub i32 0, %3
  store i32 %4, i32* %b
  %5 = load i32* %a
  %6 = icmp eq i32 0, %5
  %7 = zext i1 %6 to i32
  store i32 %7, i32* %c
  %8 = load i32* %c
  %9 = xor i32 -1, %8
  store i32 %9, i32* %b
  ret i32 0
}
