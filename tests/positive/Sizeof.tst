
struct S
{
	bool b;
	int64 s;
	[10]bool f;
}

void func()
{
	S var;

	int a = sizeof (var) + sizeof (S) + sizeof var.s + sizeof var.f[3];
}

int main()
{
	int64 a;
	[4]int64 b;

	int s = sizeof a + sizeof b;
	int z = sizeof int64 + sizeof [4]int64;

	return 0;
}

========

%S = type { i1, i64, [10 x i1] }

define void @func() {
  %var = alloca %S
  %1 = add i64 24, 24
  %2 = getelementptr %S* %var, i32 0, i32 1
  %3 = load i64* %2
  %4 = add i64 %1, 8
  %5 = getelementptr %S* %var, i32 0, i32 2
  %6 = sext i32 3 to i64
  %7 = getelementptr [10 x i1]* %5, i32 0, i64 %6
  %8 = load i1* %7
  %9 = add i64 %4, 1
  %a = alloca i32
  %10 = trunc i64 %9 to i32
  store i32 %10, i32* %a
  ret void
}

define i32 @main() {
  %a = alloca i64
  %b = alloca [4 x i64]
  %1 = add i64 8, 32
  %s = alloca i32
  %2 = trunc i64 %1 to i32
  store i32 %2, i32* %s
  %3 = add i64 8, 32
  %z = alloca i32
  %4 = trunc i64 %3 to i32
  store i32 %4, i32* %z
  ret i32 0
}
