
struct S
{
	int a, b;
}

void func()
{
	[3]int a;
	auto p1 = a$;
	auto p2 = p1$;

	p2[1] = 4;
}

void withStruct()
{
	S s;
	auto p1 = s$;
	auto p2 = p1$;

	p2.a = 10;
}

========

%S = type { i32, i32 }

define void @func() {
  %a = alloca [3 x i32]
  %p1 = alloca [3 x i32]*
  store [3 x i32]* %a, [3 x i32]** %p1
  %p2 = alloca [3 x i32]**
  store [3 x i32]** %p1, [3 x i32]*** %p2
  %1 = load [3 x i32]*** %p2
  %2 = load [3 x i32]** %1
  %3 = sext i32 1 to i64
  %4 = getelementptr [3 x i32]* %2, i32 0, i64 %3
  store i32 4, i32* %4
  ret void
}

define void @withStruct() {
  %s = alloca %S
  %p1 = alloca %S*
  store %S* %s, %S** %p1
  %p2 = alloca %S**
  store %S** %p1, %S*** %p2
  %1 = load %S*** %p2
  %2 = load %S** %1
  %3 = getelementptr %S* %2, i32 0, i32 0
  store i32 10, i32* %3
  ret void
}
