
struct Inner
{
	int one, two;
}

struct Outer
{
	Inner a, b;
	int64 c;
}

Outer func(Outer n)
{
	n.a.one = 5;
	return n;
}

int main()
{
	Outer a, b;

	a = b;
	func(b);

	return 0;
}

========

%Outer = type { %Inner, %Inner, i64 }
%Inner = type { i32, i32 }

define %Outer @func(%Outer %n) {
  %1 = alloca %Outer
  store %Outer %n, %Outer* %1
  %2 = getelementptr %Outer* %1, i32 0, i32 0
  %3 = getelementptr %Inner* %2, i32 0, i32 0
  store i32 5, i32* %3
  %4 = load %Outer* %1
  ret %Outer %4
}

define i32 @main() {
  %a = alloca %Outer
  %b = alloca %Outer
  %1 = load %Outer* %b
  store %Outer %1, %Outer* %a
  %2 = load %Outer* %b
  %3 = call %Outer @func(%Outer %2)
  ret i32 0
}
