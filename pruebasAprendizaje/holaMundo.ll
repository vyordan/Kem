; Esto es más claro y evita errores de conteo
@mensaje = constant [12 x i8] c"Hola Mundo\0A\00"
declare i32 @puts(i8*)

define i32 @main() {
    ; Obtener puntero al inicio del string
    %puntero = getelementptr [12 x i8], [12 x i8]* @mensaje, i32 0, i32 0
    call i32 @puts(i8* %puntero)
    ret i32 0
}
