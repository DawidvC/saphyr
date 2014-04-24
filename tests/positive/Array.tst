
[3]bool boolArr()
{
	[3]bool a;
	a[0] = a[1] = a[2] = 1;
	return a;
}

int calcIndex([4]int arr)
{
	auto b = boolArr();

	b[1.5] = arr[true];
	return b[0];
}

int main()
{
	[4]int a;
	[9]uint8 b;

	return a[calcIndex(a)] *  b[b[0]];
}

========

define [3 x i1] @boolArr() {
  %a = alloca [3 x i1]
  %1 = sext i32 0 to i64
  %2 = getelementptr [3 x i1]* %a, i32 0, i64 %1
  %3 = sext i32 1 to i64
  %4 = getelementptr [3 x i1]* %a, i32 0, i64 %3
  %5 = sext i32 2 to i64
  %6 = getelementptr [3 x i1]* %a, i32 0, i64 %5
  %7 = icmp ne i32 1, 0
  store i1 %7, i1* %6
  store i1 %7, i1* %4
  store i1 %7, i1* %2
  %8 = load [3 x i1]* %a
  ret [3 x i1] %8
}

define i32 @calcIndex([4 x i32] %arr) {
  %1 = alloca [4 x i32]
  store [4 x i32] %arr, [4 x i32]* %1
  %2 = call [3 x i1] @boolArr()
  %b = alloca [3 x i1]
  store [3 x i1] %2, [3 x i1]* %b
  %3 = fptosi double 1.500000e+00 to i64
  %4 = getelementptr [3 x i1]* %b, i32 0, i64 %3
  %5 = zext i1 true to i64
  %6 = getelementptr [4 x i32]* %1, i32 0, i64 %5
  %7 = load i32* %6
  %8 = icmp ne i32 %7, 0
  store i1 %8, i1* %4
  %9 = sext i32 0 to i64
  %10 = getelementptr [3 x i1]* %b, i32 0, i64 %9
  %11 = load i1* %10
  %12 = zext i1 %11 to i32
  ret i32 %12
}

define i32 @main() {
  %a = alloca [4 x i32]
  %b = alloca [9 x i8]
  %1 = load [4 x i32]* %a
  %2 = call i32 @calcIndex([4 x i32] %1)
  %3 = sext i32 %2 to i64
  %4 = getelementptr [4 x i32]* %a, i32 0, i64 %3
  %5 = load i32* %4
  %6 = sext i32 0 to i64
  %7 = getelementptr [9 x i8]* %b, i32 0, i64 %6
  %8 = load i8* %7
  %9 = zext i8 %8 to i64
  %10 = getelementptr [9 x i8]* %b, i32 0, i64 %9
  %11 = load i8* %10
  %12 = zext i8 %11 to i32
  %13 = mul i32 %5, %12
  ret i32 %13
}
