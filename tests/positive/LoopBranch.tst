
void brLoop()
{
	auto a = 9;
	while (a) {
		if (a < 4)
			break;
		a--;
	}
	auto b = 1;
	while (b) {
		while (b < 10) {
			if (b > 4)
				break 2;
		}
	}
}

void cnLoop()
{
	auto a = 9;
	while (a) {
		if (a < 4)
			continue;
		a--;
	}
	auto b = 1;
	while (b) {
		while (b < 10) {
			if (b > 4)
				continue 2;
		}
	}
}

int main()
{
	auto a = 9;
	while (a) {
		if (a < 4)
			redo;
		a--;
	}
	auto b = 1;
	while (b) {
		while (b < 10) {
			if (b > 4)
				redo 2;
		}
	}
	return 0;
}

========

define void @brLoop() {
  %a = alloca i32
  store i32 9, i32* %a
  br label %1

; <label>:1                                       ; preds = %7, %0
  %2 = load i32* %a
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %4, label %10

; <label>:4                                       ; preds = %1
  %5 = load i32* %a
  %6 = icmp slt i32 %5, 4
  br i1 %6, label %10, label %7

; <label>:7                                       ; preds = %4
  %8 = load i32* %a
  %9 = sub i32 %8, 1
  store i32 %9, i32* %a
  br label %1

; <label>:10                                      ; preds = %4, %1
  %b = alloca i32
  store i32 1, i32* %b
  br label %11

; <label>:11                                      ; preds = %14, %10
  %12 = load i32* %b
  %13 = icmp ne i32 %12, 0
  br i1 %13, label %14, label %20

; <label>:14                                      ; preds = %17, %11
  %15 = load i32* %b
  %16 = icmp slt i32 %15, 10
  br i1 %16, label %17, label %11

; <label>:17                                      ; preds = %14
  %18 = load i32* %b
  %19 = icmp sgt i32 %18, 4
  br i1 %19, label %20, label %14

; <label>:20                                      ; preds = %17, %11
  ret void
}

define void @cnLoop() {
  %a = alloca i32
  store i32 9, i32* %a
  br label %1

; <label>:1                                       ; preds = %4, %7, %0
  %2 = load i32* %a
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %4, label %10

; <label>:4                                       ; preds = %1
  %5 = load i32* %a
  %6 = icmp slt i32 %5, 4
  br i1 %6, label %1, label %7

; <label>:7                                       ; preds = %4
  %8 = load i32* %a
  %9 = sub i32 %8, 1
  store i32 %9, i32* %a
  br label %1

; <label>:10                                      ; preds = %1
  %b = alloca i32
  store i32 1, i32* %b
  br label %11

; <label>:11                                      ; preds = %14, %17, %10
  %12 = load i32* %b
  %13 = icmp ne i32 %12, 0
  br i1 %13, label %14, label %20

; <label>:14                                      ; preds = %17, %11
  %15 = load i32* %b
  %16 = icmp slt i32 %15, 10
  br i1 %16, label %17, label %11

; <label>:17                                      ; preds = %14
  %18 = load i32* %b
  %19 = icmp sgt i32 %18, 4
  br i1 %19, label %11, label %14

; <label>:20                                      ; preds = %11
  ret void
}

define i32 @main() {
  %a = alloca i32
  store i32 9, i32* %a
  br label %1

; <label>:1                                       ; preds = %7, %0
  %2 = load i32* %a
  %3 = icmp ne i32 %2, 0
  br i1 %3, label %4, label %10

; <label>:4                                       ; preds = %4, %1
  %5 = load i32* %a
  %6 = icmp slt i32 %5, 4
  br i1 %6, label %4, label %7

; <label>:7                                       ; preds = %4
  %8 = load i32* %a
  %9 = sub i32 %8, 1
  store i32 %9, i32* %a
  br label %1

; <label>:10                                      ; preds = %1
  %b = alloca i32
  store i32 1, i32* %b
  br label %11

; <label>:11                                      ; preds = %14, %10
  %12 = load i32* %b
  %13 = icmp ne i32 %12, 0
  br i1 %13, label %14, label %20

; <label>:14                                      ; preds = %17, %17, %11
  %15 = load i32* %b
  %16 = icmp slt i32 %15, 10
  br i1 %16, label %17, label %11

; <label>:17                                      ; preds = %14
  %18 = load i32* %b
  %19 = icmp sgt i32 %18, 4
  br i1 %19, label %14, label %14

; <label>:20                                      ; preds = %11
  ret i32 0
}
