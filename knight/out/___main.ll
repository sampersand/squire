; Prelude
target triple = "arm64-apple-macosx12.0.0"
%bool = type i64 ; used to be i8, this is just to simplify interfaces
%num = type i64
%struct.builtin.str = type { i8*, i64 } ; (ptr, len)
%struct.builtin.any = type { i8*, i64 } ; (ptr, type)
%struct.builtin.list = type { i8*, i64, i64 } ; (ptr, len, cap)
%struct.builtin.dict = type { %struct.builtin.dict_eles*, i64, i64, i1 (i64, i64)* } ; (eles, len, cap, eql)

%struct.builtin.dict_eles = type { i64, i64 }
%struct.builtin.io = type { %struct.builtin.str*, %struct.builtin.str*, i8* } ; (name, mode, file)

; Number builtins
declare %struct.builtin.str* @num_to_str(%num %0) 
declare %num @powll(%num %0, %num %1)

; List builtins builtins
declare %struct.builtin.list* @allocate_list(i64 %0) 
declare %struct.builtin.list* @concat_lists(%struct.builtin.list* %0, %struct.builtin.list* %1)
declare %struct.builtin.list* @repeat_list(%struct.builtin.list* %0, %num %1)
declare %bool @insert_into_list(%struct.builtin.list* %0, i64 %1, i8* %2)
declare %bool @delete_from_list(%struct.builtin.list* %0, i8* %1, i64 %2)
declare void @push_into_list(%struct.builtin.list* %0, i8* %1)
declare void @pop_from_list(%struct.builtin.list* %0, i8* %1)

; Dict builtins builtins
declare %struct.builtin.dict* @allocate_dict(i64 %0, i1 (i64,i64)* %1) 
declare i1 @fetch_from_dict(%struct.builtin.dict* %0, i8* %1, i8* %2)
declare void @insert_into_dict(%struct.builtin.dict* %0, i8* %1, i8* %2)
declare i1 @has_key(%struct.builtin.dict* %0, i8* %1)

declare i1 @compare_val(i64 %0, i64 %1)
declare i1 @compare_str(i64 %0, i64 %1)
declare i1 @compare_list(i64 %0, i64 %1)
declare i1 @compare_dict(i64 %0, i64 %1)

; String builtins
declare %struct.builtin.str* @allocate_str(i64 %0) 
declare %num @str_to_num(%struct.builtin.str* %0) 
declare %struct.builtin.str* @concat_strs(%struct.builtin.str* %0, %struct.builtin.str* %1) 
declare %struct.builtin.str* @repeat_str(%struct.builtin.str* %0, %num %1) 
declare i32 @compare_strs(%struct.builtin.str* %0, %struct.builtin.str* %1) 
declare %struct.builtin.str* @substr(%struct.builtin.str* %0, i64 %1, i64 %2)
declare %struct.builtin.str* @ascii_to_str(%num %0)
declare %num @str_to_ascii(%struct.builtin.str* %0)

; IO Builtins
declare %struct.builtin.io* @open_io(%struct.builtin.str* %0, %struct.builtin.str* %1)
declare %struct.builtin.str* @readline_io(%struct.builtin.io* %0)
declare %struct.builtin.str* @readall_io(%struct.builtin.io* %0)
declare void @write_io(%struct.builtin.io* %0, %struct.builtin.str* %1)

; Misc builtins
declare i8* @xmalloc(i64 %0)
declare void @print_str(%struct.builtin.str* %0) 
declare void @println_str(%struct.builtin.str* %0) 
declare void @quit(%num %0) noreturn 
declare void @abort_msg(%struct.builtin.str* %0) noreturn 
declare %struct.builtin.str* @prompt() 
declare %num @random_()

; Inline stuff


; Struct declarations


; Enum declarations


; Global declarations


; External declarations


; String declarations


; Functions


declare %num @fn.user.main(%struct.builtin.list* %0)

define %num @main() {
  %1 = call %num @fn.user.main(%struct.builtin.list* null);
  ret %num %1;
}

