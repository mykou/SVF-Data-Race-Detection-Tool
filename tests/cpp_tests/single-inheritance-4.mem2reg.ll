; ModuleID = 'single-inheritance-4.ll'
source_filename = "single-inheritance-4.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%class.B = type { %class.A }
%class.A = type { i32 (...)** }

$_ZN1BC2Ev = comdat any

$_ZN1AC2Ev = comdat any

$_ZN1B1fEPi = comdat any

$_ZN1A1fEPi = comdat any

$_ZTV1B = comdat any

$_ZTS1B = comdat any

$_ZTS1A = comdat any

$_ZTI1A = comdat any

$_ZTI1B = comdat any

$_ZTV1A = comdat any

@.str = private unnamed_addr constant [2 x i8] c"\0A\00", align 1
@.str.1 = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@.str.2 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@.str.3 = private unnamed_addr constant [25 x i8] c"Press ENTER to continue\0A\00", align 1
@global_obj = global i32 0, align 4, !dbg !0
@global_ptr = global i32* @global_obj, align 8, !dbg !6
@_ZTV1B = linkonce_odr unnamed_addr constant { [3 x i8*] } { [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8*, i8* }* @_ZTI1B to i8*), i8* bitcast (void (%class.B*, i32*)* @_ZN1B1fEPi to i8*)] }, comdat, align 8
@_ZTVN10__cxxabiv120__si_class_type_infoE = external global i8*
@_ZTS1B = linkonce_odr constant [3 x i8] c"1B\00", comdat
@_ZTVN10__cxxabiv117__class_type_infoE = external global i8*
@_ZTS1A = linkonce_odr constant [3 x i8] c"1A\00", comdat
@_ZTI1A = linkonce_odr constant { i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv117__class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1A, i32 0, i32 0) }, comdat
@_ZTI1B = linkonce_odr constant { i8*, i8*, i8* } { i8* bitcast (i8** getelementptr inbounds (i8*, i8** @_ZTVN10__cxxabiv120__si_class_type_infoE, i64 2) to i8*), i8* getelementptr inbounds ([3 x i8], [3 x i8]* @_ZTS1B, i32 0, i32 0), i8* bitcast ({ i8*, i8* }* @_ZTI1A to i8*) }, comdat
@_ZTV1A = linkonce_odr unnamed_addr constant { [3 x i8*] } { [3 x i8*] [i8* null, i8* bitcast ({ i8*, i8* }* @_ZTI1A to i8*), i8* bitcast (void (%class.A*, i32*)* @_ZN1A1fEPi to i8*)] }, comdat, align 8

; Function Attrs: noinline uwtable
define void @_Z9MUSTALIASPvS_(i8*, i8*) #0 !dbg !13 {
  call void @llvm.dbg.value(metadata i8* %0, i64 0, metadata !18, metadata !19), !dbg !20
  call void @llvm.dbg.value(metadata i8* %1, i64 0, metadata !21, metadata !19), !dbg !22
  %3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i32 0, i32 0)), !dbg !23
  ret void, !dbg !24
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

declare i32 @printf(i8*, ...) #2

; Function Attrs: noinline uwtable
define void @_Z12PARTAILALIASPvS_(i8*, i8*) #0 !dbg !25 {
  call void @llvm.dbg.value(metadata i8* %0, i64 0, metadata !26, metadata !19), !dbg !27
  call void @llvm.dbg.value(metadata i8* %1, i64 0, metadata !28, metadata !19), !dbg !29
  %3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i32 0, i32 0)), !dbg !30
  ret void, !dbg !31
}

; Function Attrs: noinline uwtable
define void @_Z8MAYALIASPvS_(i8*, i8*) #0 !dbg !32 {
  call void @llvm.dbg.value(metadata i8* %0, i64 0, metadata !33, metadata !19), !dbg !34
  call void @llvm.dbg.value(metadata i8* %1, i64 0, metadata !35, metadata !19), !dbg !36
  %3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i32 0, i32 0)), !dbg !37
  ret void, !dbg !38
}

; Function Attrs: noinline uwtable
define void @_Z7NOALIASPvS_(i8*, i8*) #0 !dbg !39 {
  call void @llvm.dbg.value(metadata i8* %0, i64 0, metadata !40, metadata !19), !dbg !41
  call void @llvm.dbg.value(metadata i8* %1, i64 0, metadata !42, metadata !19), !dbg !43
  %3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i32 0, i32 0)), !dbg !44
  ret void, !dbg !45
}

; Function Attrs: noinline uwtable
define void @_Z21EXPECTEDFAIL_MAYALIASPvS_(i8*, i8*) #0 !dbg !46 {
  call void @llvm.dbg.value(metadata i8* %0, i64 0, metadata !47, metadata !19), !dbg !48
  call void @llvm.dbg.value(metadata i8* %1, i64 0, metadata !49, metadata !19), !dbg !50
  %3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i32 0, i32 0)), !dbg !51
  ret void, !dbg !52
}

; Function Attrs: noinline uwtable
define void @_Z20EXPECTEDFAIL_NOALIASPvS_(i8*, i8*) #0 !dbg !53 {
  call void @llvm.dbg.value(metadata i8* %0, i64 0, metadata !54, metadata !19), !dbg !55
  call void @llvm.dbg.value(metadata i8* %1, i64 0, metadata !56, metadata !19), !dbg !57
  %3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i32 0, i32 0)), !dbg !58
  ret void, !dbg !59
}

; Function Attrs: noinline nounwind uwtable
define i8* @_Z10SAFEMALLOCi(i32) #3 !dbg !60 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !63, metadata !19), !dbg !64
  %2 = sext i32 %0 to i64, !dbg !65
  %3 = call noalias i8* @malloc(i64 %2) #7, !dbg !66
  ret i8* %3, !dbg !67
}

; Function Attrs: nounwind
declare noalias i8* @malloc(i64) #4

; Function Attrs: noinline nounwind uwtable
define i8* @_Z9PLKMALLOCi(i32) #3 !dbg !68 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !69, metadata !19), !dbg !70
  %2 = sext i32 %0 to i64, !dbg !71
  %3 = call noalias i8* @malloc(i64 %2) #7, !dbg !72
  ret i8* %3, !dbg !73
}

; Function Attrs: noinline nounwind uwtable
define i8* @_Z9NFRMALLOCi(i32) #3 !dbg !74 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !75, metadata !19), !dbg !76
  %2 = sext i32 %0 to i64, !dbg !77
  %3 = call noalias i8* @malloc(i64 %2) #7, !dbg !78
  ret i8* %3, !dbg !79
}

; Function Attrs: noinline nounwind uwtable
define i8* @_Z9CLKMALLOCi(i32) #3 !dbg !80 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !81, metadata !19), !dbg !82
  %2 = sext i32 %0 to i64, !dbg !83
  %3 = call noalias i8* @malloc(i64 %2) #7, !dbg !84
  ret i8* %3, !dbg !85
}

; Function Attrs: noinline nounwind uwtable
define i8* @_Z9NFRLEAKFPi(i32) #3 !dbg !86 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !87, metadata !19), !dbg !88
  %2 = sext i32 %0 to i64, !dbg !89
  %3 = call noalias i8* @malloc(i64 %2) #7, !dbg !90
  ret i8* %3, !dbg !91
}

; Function Attrs: noinline nounwind uwtable
define i8* @_Z9PLKLEAKFPi(i32) #3 !dbg !92 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !93, metadata !19), !dbg !94
  %2 = sext i32 %0 to i64, !dbg !95
  %3 = call noalias i8* @malloc(i64 %2) #7, !dbg !96
  ret i8* %3, !dbg !97
}

; Function Attrs: noinline nounwind uwtable
define i8* @_Z6LEAKFNi(i32) #3 !dbg !98 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !99, metadata !19), !dbg !100
  %2 = sext i32 %0 to i64, !dbg !101
  %3 = call noalias i8* @malloc(i64 %2) #7, !dbg !102
  ret i8* %3, !dbg !103
}

; Function Attrs: noinline uwtable
define void @RC_ACCESS(i32, i32) #0 !dbg !104 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !107, metadata !19), !dbg !108
  call void @llvm.dbg.value(metadata i32 %1, i64 0, metadata !109, metadata !19), !dbg !110
  %3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([1 x i8], [1 x i8]* @.str.1, i32 0, i32 0)), !dbg !111
  ret void, !dbg !112
}

; Function Attrs: noinline nounwind uwtable
define void @_Z10CXT_THREADiPc(i32, i8*) #3 !dbg !113 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !118, metadata !19), !dbg !119
  call void @llvm.dbg.value(metadata i8* %1, i64 0, metadata !120, metadata !19), !dbg !121
  ret void, !dbg !122
}

; Function Attrs: noinline nounwind uwtable
define void @_Z10TCT_ACCESSiPc(i32, i8*) #3 !dbg !123 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !124, metadata !19), !dbg !125
  call void @llvm.dbg.value(metadata i8* %1, i64 0, metadata !126, metadata !19), !dbg !127
  ret void, !dbg !128
}

; Function Attrs: noinline nounwind uwtable
define void @_Z15INTERLEV_ACCESSiPcS_(i32, i8*, i8*) #3 !dbg !129 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !132, metadata !19), !dbg !133
  call void @llvm.dbg.value(metadata i8* %1, i64 0, metadata !134, metadata !19), !dbg !135
  call void @llvm.dbg.value(metadata i8* %2, i64 0, metadata !136, metadata !19), !dbg !137
  ret void, !dbg !138
}

; Function Attrs: noinline uwtable
define void @_Z5PAUSEPc(i8*) #0 !dbg !139 {
  call void @llvm.dbg.value(metadata i8* %0, i64 0, metadata !142, metadata !19), !dbg !143
  %2 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.2, i32 0, i32 0), i8* %0), !dbg !144
  %3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([25 x i8], [25 x i8]* @.str.3, i32 0, i32 0)), !dbg !145
  %4 = call i32 @getchar(), !dbg !146
  ret void, !dbg !147
}

declare i32 @getchar() #2

; Function Attrs: noinline norecurse uwtable
define i32 @main(i32, i8**) #5 !dbg !148 {
  call void @llvm.dbg.value(metadata i32 %0, i64 0, metadata !152, metadata !19), !dbg !153
  call void @llvm.dbg.value(metadata i8** %1, i64 0, metadata !154, metadata !19), !dbg !155
  call void @llvm.dbg.value(metadata i32* @global_obj, i64 0, metadata !156, metadata !19), !dbg !157
  %3 = call i8* @_Znwm(i64 8) #8, !dbg !158
  %4 = bitcast i8* %3 to %class.B*, !dbg !158
  call void @_ZN1BC2Ev(%class.B* %4) #7, !dbg !159
  %5 = bitcast %class.B* %4 to %class.A*, !dbg !158
  call void @llvm.dbg.value(metadata %class.A* %5, i64 0, metadata !161, metadata !19), !dbg !174
  %6 = bitcast %class.A* %5 to void (%class.A*, i32*)***, !dbg !175
  %7 = load void (%class.A*, i32*)**, void (%class.A*, i32*)*** %6, align 8, !dbg !175
  %8 = getelementptr inbounds void (%class.A*, i32*)*, void (%class.A*, i32*)** %7, i64 0, !dbg !175
  %9 = load void (%class.A*, i32*)*, void (%class.A*, i32*)** %8, align 8, !dbg !175
  call void %9(%class.A* %5, i32* @global_obj), !dbg !175
  ret i32 0, !dbg !176
}

; Function Attrs: nobuiltin
declare noalias i8* @_Znwm(i64) #6

; Function Attrs: noinline nounwind uwtable
define linkonce_odr void @_ZN1BC2Ev(%class.B*) unnamed_addr #3 comdat align 2 !dbg !177 {
  call void @llvm.dbg.value(metadata %class.B* %0, i64 0, metadata !188, metadata !19), !dbg !190
  %2 = bitcast %class.B* %0 to %class.A*, !dbg !191
  call void @_ZN1AC2Ev(%class.A* %2) #7, !dbg !191
  %3 = bitcast %class.B* %0 to i32 (...)***, !dbg !191
  store i32 (...)** bitcast (i8** getelementptr inbounds ({ [3 x i8*] }, { [3 x i8*] }* @_ZTV1B, i32 0, inrange i32 0, i32 2) to i32 (...)**), i32 (...)*** %3, align 8, !dbg !191
  ret void, !dbg !191
}

; Function Attrs: noinline nounwind uwtable
define linkonce_odr void @_ZN1AC2Ev(%class.A*) unnamed_addr #3 comdat align 2 !dbg !192 {
  call void @llvm.dbg.value(metadata %class.A* %0, i64 0, metadata !196, metadata !19), !dbg !197
  %2 = bitcast %class.A* %0 to i32 (...)***, !dbg !198
  store i32 (...)** bitcast (i8** getelementptr inbounds ({ [3 x i8*] }, { [3 x i8*] }* @_ZTV1A, i32 0, inrange i32 0, i32 2) to i32 (...)**), i32 (...)*** %2, align 8, !dbg !198
  ret void, !dbg !198
}

; Function Attrs: noinline uwtable
define linkonce_odr void @_ZN1B1fEPi(%class.B*, i32*) unnamed_addr #0 comdat align 2 !dbg !199 {
  call void @llvm.dbg.value(metadata %class.B* %0, i64 0, metadata !200, metadata !19), !dbg !201
  call void @llvm.dbg.value(metadata i32* %1, i64 0, metadata !202, metadata !19), !dbg !203
  %3 = load i32*, i32** @global_ptr, align 8, !dbg !204
  %4 = bitcast i32* %3 to i8*, !dbg !204
  %5 = bitcast i32* %1 to i8*, !dbg !205
  call void @_Z9MUSTALIASPvS_(i8* %4, i8* %5), !dbg !206
  ret void, !dbg !207
}

; Function Attrs: noinline uwtable
define linkonce_odr void @_ZN1A1fEPi(%class.A*, i32*) unnamed_addr #0 comdat align 2 !dbg !208 {
  call void @llvm.dbg.value(metadata %class.A* %0, i64 0, metadata !209, metadata !19), !dbg !210
  call void @llvm.dbg.value(metadata i32* %1, i64 0, metadata !211, metadata !19), !dbg !212
  %3 = load i32*, i32** @global_ptr, align 8, !dbg !213
  %4 = bitcast i32* %3 to i8*, !dbg !213
  %5 = bitcast i32* %1 to i8*, !dbg !214
  call void @_Z8MAYALIASPvS_(i8* %4, i8* %5), !dbg !215
  ret void, !dbg !216
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata, metadata) #1

attributes #0 = { noinline uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { noinline norecurse uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #6 = { nobuiltin "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #7 = { nounwind }
attributes #8 = { builtin }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!10, !11}
!llvm.ident = !{!12}

!0 = !DIGlobalVariableExpression(var: !1)
!1 = distinct !DIGlobalVariable(name: "global_obj", scope: !2, file: !3, line: 3, type: !9, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !3, producer: "clang version 4.0.0 (tags/RELEASE_400/final)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5)
!3 = !DIFile(filename: "single-inheritance-4.cpp", directory: "/home/ysui/pta/tests/cpp_tests")
!4 = !{}
!5 = !{!0, !6}
!6 = !DIGlobalVariableExpression(var: !7)
!7 = distinct !DIGlobalVariable(name: "global_ptr", scope: !2, file: !3, line: 4, type: !8, isLocal: false, isDefinition: true)
!8 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !9, size: 64)
!9 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!10 = !{i32 2, !"Dwarf Version", i32 4}
!11 = !{i32 2, !"Debug Info Version", i32 3}
!12 = !{!"clang version 4.0.0 (tags/RELEASE_400/final)"}
!13 = distinct !DISubprogram(name: "MUSTALIAS", linkageName: "_Z9MUSTALIASPvS_", scope: !14, file: !14, line: 4, type: !15, isLocal: false, isDefinition: true, scopeLine: 4, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!14 = !DIFile(filename: "../aliascheck.h", directory: "/home/ysui/pta/tests/cpp_tests")
!15 = !DISubroutineType(types: !16)
!16 = !{null, !17, !17}
!17 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
!18 = !DILocalVariable(name: "p", arg: 1, scope: !13, file: !14, line: 4, type: !17)
!19 = !DIExpression()
!20 = !DILocation(line: 4, column: 22, scope: !13)
!21 = !DILocalVariable(name: "q", arg: 2, scope: !13, file: !14, line: 4, type: !17)
!22 = !DILocation(line: 4, column: 31, scope: !13)
!23 = !DILocation(line: 5, column: 3, scope: !13)
!24 = !DILocation(line: 6, column: 1, scope: !13)
!25 = distinct !DISubprogram(name: "PARTAILALIAS", linkageName: "_Z12PARTAILALIASPvS_", scope: !14, file: !14, line: 8, type: !15, isLocal: false, isDefinition: true, scopeLine: 8, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!26 = !DILocalVariable(name: "p", arg: 1, scope: !25, file: !14, line: 8, type: !17)
!27 = !DILocation(line: 8, column: 25, scope: !25)
!28 = !DILocalVariable(name: "q", arg: 2, scope: !25, file: !14, line: 8, type: !17)
!29 = !DILocation(line: 8, column: 34, scope: !25)
!30 = !DILocation(line: 9, column: 3, scope: !25)
!31 = !DILocation(line: 10, column: 1, scope: !25)
!32 = distinct !DISubprogram(name: "MAYALIAS", linkageName: "_Z8MAYALIASPvS_", scope: !14, file: !14, line: 12, type: !15, isLocal: false, isDefinition: true, scopeLine: 12, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!33 = !DILocalVariable(name: "p", arg: 1, scope: !32, file: !14, line: 12, type: !17)
!34 = !DILocation(line: 12, column: 21, scope: !32)
!35 = !DILocalVariable(name: "q", arg: 2, scope: !32, file: !14, line: 12, type: !17)
!36 = !DILocation(line: 12, column: 30, scope: !32)
!37 = !DILocation(line: 13, column: 3, scope: !32)
!38 = !DILocation(line: 14, column: 1, scope: !32)
!39 = distinct !DISubprogram(name: "NOALIAS", linkageName: "_Z7NOALIASPvS_", scope: !14, file: !14, line: 16, type: !15, isLocal: false, isDefinition: true, scopeLine: 16, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!40 = !DILocalVariable(name: "p", arg: 1, scope: !39, file: !14, line: 16, type: !17)
!41 = !DILocation(line: 16, column: 20, scope: !39)
!42 = !DILocalVariable(name: "q", arg: 2, scope: !39, file: !14, line: 16, type: !17)
!43 = !DILocation(line: 16, column: 29, scope: !39)
!44 = !DILocation(line: 17, column: 3, scope: !39)
!45 = !DILocation(line: 18, column: 1, scope: !39)
!46 = distinct !DISubprogram(name: "EXPECTEDFAIL_MAYALIAS", linkageName: "_Z21EXPECTEDFAIL_MAYALIASPvS_", scope: !14, file: !14, line: 20, type: !15, isLocal: false, isDefinition: true, scopeLine: 20, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!47 = !DILocalVariable(name: "p", arg: 1, scope: !46, file: !14, line: 20, type: !17)
!48 = !DILocation(line: 20, column: 34, scope: !46)
!49 = !DILocalVariable(name: "q", arg: 2, scope: !46, file: !14, line: 20, type: !17)
!50 = !DILocation(line: 20, column: 43, scope: !46)
!51 = !DILocation(line: 21, column: 3, scope: !46)
!52 = !DILocation(line: 22, column: 1, scope: !46)
!53 = distinct !DISubprogram(name: "EXPECTEDFAIL_NOALIAS", linkageName: "_Z20EXPECTEDFAIL_NOALIASPvS_", scope: !14, file: !14, line: 24, type: !15, isLocal: false, isDefinition: true, scopeLine: 24, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!54 = !DILocalVariable(name: "p", arg: 1, scope: !53, file: !14, line: 24, type: !17)
!55 = !DILocation(line: 24, column: 33, scope: !53)
!56 = !DILocalVariable(name: "q", arg: 2, scope: !53, file: !14, line: 24, type: !17)
!57 = !DILocation(line: 24, column: 42, scope: !53)
!58 = !DILocation(line: 25, column: 3, scope: !53)
!59 = !DILocation(line: 26, column: 1, scope: !53)
!60 = distinct !DISubprogram(name: "SAFEMALLOC", linkageName: "_Z10SAFEMALLOCi", scope: !14, file: !14, line: 29, type: !61, isLocal: false, isDefinition: true, scopeLine: 29, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!61 = !DISubroutineType(types: !62)
!62 = !{!17, !9}
!63 = !DILocalVariable(name: "n", arg: 1, scope: !60, file: !14, line: 29, type: !9)
!64 = !DILocation(line: 29, column: 22, scope: !60)
!65 = !DILocation(line: 30, column: 17, scope: !60)
!66 = !DILocation(line: 30, column: 10, scope: !60)
!67 = !DILocation(line: 30, column: 3, scope: !60)
!68 = distinct !DISubprogram(name: "PLKMALLOC", linkageName: "_Z9PLKMALLOCi", scope: !14, file: !14, line: 33, type: !61, isLocal: false, isDefinition: true, scopeLine: 33, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!69 = !DILocalVariable(name: "n", arg: 1, scope: !68, file: !14, line: 33, type: !9)
!70 = !DILocation(line: 33, column: 21, scope: !68)
!71 = !DILocation(line: 34, column: 17, scope: !68)
!72 = !DILocation(line: 34, column: 10, scope: !68)
!73 = !DILocation(line: 34, column: 3, scope: !68)
!74 = distinct !DISubprogram(name: "NFRMALLOC", linkageName: "_Z9NFRMALLOCi", scope: !14, file: !14, line: 37, type: !61, isLocal: false, isDefinition: true, scopeLine: 37, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!75 = !DILocalVariable(name: "n", arg: 1, scope: !74, file: !14, line: 37, type: !9)
!76 = !DILocation(line: 37, column: 21, scope: !74)
!77 = !DILocation(line: 38, column: 17, scope: !74)
!78 = !DILocation(line: 38, column: 10, scope: !74)
!79 = !DILocation(line: 38, column: 3, scope: !74)
!80 = distinct !DISubprogram(name: "CLKMALLOC", linkageName: "_Z9CLKMALLOCi", scope: !14, file: !14, line: 41, type: !61, isLocal: false, isDefinition: true, scopeLine: 41, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!81 = !DILocalVariable(name: "n", arg: 1, scope: !80, file: !14, line: 41, type: !9)
!82 = !DILocation(line: 41, column: 21, scope: !80)
!83 = !DILocation(line: 42, column: 17, scope: !80)
!84 = !DILocation(line: 42, column: 10, scope: !80)
!85 = !DILocation(line: 42, column: 3, scope: !80)
!86 = distinct !DISubprogram(name: "NFRLEAKFP", linkageName: "_Z9NFRLEAKFPi", scope: !14, file: !14, line: 45, type: !61, isLocal: false, isDefinition: true, scopeLine: 45, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!87 = !DILocalVariable(name: "n", arg: 1, scope: !86, file: !14, line: 45, type: !9)
!88 = !DILocation(line: 45, column: 21, scope: !86)
!89 = !DILocation(line: 46, column: 17, scope: !86)
!90 = !DILocation(line: 46, column: 10, scope: !86)
!91 = !DILocation(line: 46, column: 3, scope: !86)
!92 = distinct !DISubprogram(name: "PLKLEAKFP", linkageName: "_Z9PLKLEAKFPi", scope: !14, file: !14, line: 49, type: !61, isLocal: false, isDefinition: true, scopeLine: 49, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!93 = !DILocalVariable(name: "n", arg: 1, scope: !92, file: !14, line: 49, type: !9)
!94 = !DILocation(line: 49, column: 21, scope: !92)
!95 = !DILocation(line: 50, column: 17, scope: !92)
!96 = !DILocation(line: 50, column: 10, scope: !92)
!97 = !DILocation(line: 50, column: 3, scope: !92)
!98 = distinct !DISubprogram(name: "LEAKFN", linkageName: "_Z6LEAKFNi", scope: !14, file: !14, line: 53, type: !61, isLocal: false, isDefinition: true, scopeLine: 53, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!99 = !DILocalVariable(name: "n", arg: 1, scope: !98, file: !14, line: 53, type: !9)
!100 = !DILocation(line: 53, column: 18, scope: !98)
!101 = !DILocation(line: 54, column: 17, scope: !98)
!102 = !DILocation(line: 54, column: 10, scope: !98)
!103 = !DILocation(line: 54, column: 3, scope: !98)
!104 = distinct !DISubprogram(name: "RC_ACCESS", scope: !14, file: !14, line: 67, type: !105, isLocal: false, isDefinition: true, scopeLine: 67, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!105 = !DISubroutineType(types: !106)
!106 = !{null, !9, !9}
!107 = !DILocalVariable(name: "id", arg: 1, scope: !104, file: !14, line: 67, type: !9)
!108 = !DILocation(line: 67, column: 20, scope: !104)
!109 = !DILocalVariable(name: "flags", arg: 2, scope: !104, file: !14, line: 67, type: !9)
!110 = !DILocation(line: 67, column: 28, scope: !104)
!111 = !DILocation(line: 68, column: 3, scope: !104)
!112 = !DILocation(line: 69, column: 1, scope: !104)
!113 = distinct !DISubprogram(name: "CXT_THREAD", linkageName: "_Z10CXT_THREADiPc", scope: !14, file: !14, line: 85, type: !114, isLocal: false, isDefinition: true, scopeLine: 85, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!114 = !DISubroutineType(types: !115)
!115 = !{null, !9, !116}
!116 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !117, size: 64)
!117 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!118 = !DILocalVariable(name: "thdid", arg: 1, scope: !113, file: !14, line: 85, type: !9)
!119 = !DILocation(line: 85, column: 21, scope: !113)
!120 = !DILocalVariable(name: "cxt", arg: 2, scope: !113, file: !14, line: 85, type: !116)
!121 = !DILocation(line: 85, column: 34, scope: !113)
!122 = !DILocation(line: 87, column: 1, scope: !113)
!123 = distinct !DISubprogram(name: "TCT_ACCESS", linkageName: "_Z10TCT_ACCESSiPc", scope: !14, file: !14, line: 88, type: !114, isLocal: false, isDefinition: true, scopeLine: 88, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!124 = !DILocalVariable(name: "thdid", arg: 1, scope: !123, file: !14, line: 88, type: !9)
!125 = !DILocation(line: 88, column: 21, scope: !123)
!126 = !DILocalVariable(name: "cxt", arg: 2, scope: !123, file: !14, line: 88, type: !116)
!127 = !DILocation(line: 88, column: 33, scope: !123)
!128 = !DILocation(line: 90, column: 1, scope: !123)
!129 = distinct !DISubprogram(name: "INTERLEV_ACCESS", linkageName: "_Z15INTERLEV_ACCESSiPcS_", scope: !14, file: !14, line: 91, type: !130, isLocal: false, isDefinition: true, scopeLine: 91, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!130 = !DISubroutineType(types: !131)
!131 = !{null, !9, !116, !116}
!132 = !DILocalVariable(name: "thdid", arg: 1, scope: !129, file: !14, line: 91, type: !9)
!133 = !DILocation(line: 91, column: 26, scope: !129)
!134 = !DILocalVariable(name: "cxt", arg: 2, scope: !129, file: !14, line: 91, type: !116)
!135 = !DILocation(line: 91, column: 38, scope: !129)
!136 = !DILocalVariable(name: "lev", arg: 3, scope: !129, file: !14, line: 91, type: !116)
!137 = !DILocation(line: 91, column: 49, scope: !129)
!138 = !DILocation(line: 93, column: 1, scope: !129)
!139 = distinct !DISubprogram(name: "PAUSE", linkageName: "_Z5PAUSEPc", scope: !14, file: !14, line: 95, type: !140, isLocal: false, isDefinition: true, scopeLine: 95, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!140 = !DISubroutineType(types: !141)
!141 = !{null, !116}
!142 = !DILocalVariable(name: "str", arg: 1, scope: !139, file: !14, line: 95, type: !116)
!143 = !DILocation(line: 95, column: 18, scope: !139)
!144 = !DILocation(line: 96, column: 3, scope: !139)
!145 = !DILocation(line: 97, column: 3, scope: !139)
!146 = !DILocation(line: 98, column: 3, scope: !139)
!147 = !DILocation(line: 99, column: 1, scope: !139)
!148 = distinct !DISubprogram(name: "main", scope: !3, file: !3, line: 19, type: !149, isLocal: false, isDefinition: true, scopeLine: 20, flags: DIFlagPrototyped, isOptimized: false, unit: !2, variables: !4)
!149 = !DISubroutineType(types: !150)
!150 = !{!9, !9, !151}
!151 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !116, size: 64)
!152 = !DILocalVariable(name: "argc", arg: 1, scope: !148, file: !3, line: 19, type: !9)
!153 = !DILocation(line: 19, column: 14, scope: !148)
!154 = !DILocalVariable(name: "argv", arg: 2, scope: !148, file: !3, line: 19, type: !151)
!155 = !DILocation(line: 19, column: 27, scope: !148)
!156 = !DILocalVariable(name: "ptr", scope: !148, file: !3, line: 21, type: !8)
!157 = !DILocation(line: 21, column: 8, scope: !148)
!158 = !DILocation(line: 23, column: 11, scope: !148)
!159 = !DILocation(line: 23, column: 15, scope: !160)
!160 = !DILexicalBlockFile(scope: !148, file: !3, discriminator: 1)
!161 = !DILocalVariable(name: "pb", scope: !148, file: !3, line: 23, type: !162)
!162 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !163, size: 64)
!163 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "A", file: !3, line: 6, size: 64, elements: !164, vtableHolder: !163, identifier: "_ZTS1A")
!164 = !{!165, !170}
!165 = !DIDerivedType(tag: DW_TAG_member, name: "_vptr$A", scope: !3, file: !3, baseType: !166, size: 64, flags: DIFlagArtificial)
!166 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !167, size: 64)
!167 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "__vtbl_ptr_type", baseType: !168, size: 64)
!168 = !DISubroutineType(types: !169)
!169 = !{!9}
!170 = !DISubprogram(name: "f", linkageName: "_ZN1A1fEPi", scope: !163, file: !3, line: 8, type: !171, isLocal: false, isDefinition: false, scopeLine: 8, containingType: !163, virtuality: DW_VIRTUALITY_virtual, virtualIndex: 0, flags: DIFlagPublic | DIFlagPrototyped, isOptimized: false)
!171 = !DISubroutineType(types: !172)
!172 = !{null, !173, !8}
!173 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !163, size: 64, flags: DIFlagArtificial | DIFlagObjectPointer)
!174 = !DILocation(line: 23, column: 6, scope: !148)
!175 = !DILocation(line: 24, column: 7, scope: !148)
!176 = !DILocation(line: 26, column: 3, scope: !148)
!177 = distinct !DISubprogram(name: "B", linkageName: "_ZN1BC2Ev", scope: !178, file: !3, line: 13, type: !185, isLocal: false, isDefinition: true, scopeLine: 13, flags: DIFlagArtificial | DIFlagPrototyped, isOptimized: false, unit: !2, declaration: !187, variables: !4)
!178 = distinct !DICompositeType(tag: DW_TAG_class_type, name: "B", file: !3, line: 13, size: 64, elements: !179, vtableHolder: !163, identifier: "_ZTS1B")
!179 = !{!180, !181}
!180 = !DIDerivedType(tag: DW_TAG_inheritance, scope: !178, baseType: !163, flags: DIFlagPublic)
!181 = !DISubprogram(name: "f", linkageName: "_ZN1B1fEPi", scope: !178, file: !3, line: 14, type: !182, isLocal: false, isDefinition: false, scopeLine: 14, containingType: !178, virtuality: DW_VIRTUALITY_virtual, virtualIndex: 0, flags: DIFlagPrototyped, isOptimized: false)
!182 = !DISubroutineType(types: !183)
!183 = !{null, !184, !8}
!184 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !178, size: 64, flags: DIFlagArtificial | DIFlagObjectPointer)
!185 = !DISubroutineType(types: !186)
!186 = !{null, !184}
!187 = !DISubprogram(name: "B", scope: !178, type: !185, isLocal: false, isDefinition: false, flags: DIFlagPublic | DIFlagArtificial | DIFlagPrototyped, isOptimized: false)
!188 = !DILocalVariable(name: "this", arg: 1, scope: !177, type: !189, flags: DIFlagArtificial | DIFlagObjectPointer)
!189 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !178, size: 64)
!190 = !DILocation(line: 0, scope: !177)
!191 = !DILocation(line: 13, column: 7, scope: !177)
!192 = distinct !DISubprogram(name: "A", linkageName: "_ZN1AC2Ev", scope: !163, file: !3, line: 6, type: !193, isLocal: false, isDefinition: true, scopeLine: 6, flags: DIFlagArtificial | DIFlagPrototyped, isOptimized: false, unit: !2, declaration: !195, variables: !4)
!193 = !DISubroutineType(types: !194)
!194 = !{null, !173}
!195 = !DISubprogram(name: "A", scope: !163, type: !193, isLocal: false, isDefinition: false, flags: DIFlagPublic | DIFlagArtificial | DIFlagPrototyped, isOptimized: false)
!196 = !DILocalVariable(name: "this", arg: 1, scope: !192, type: !162, flags: DIFlagArtificial | DIFlagObjectPointer)
!197 = !DILocation(line: 0, scope: !192)
!198 = !DILocation(line: 6, column: 7, scope: !192)
!199 = distinct !DISubprogram(name: "f", linkageName: "_ZN1B1fEPi", scope: !178, file: !3, line: 14, type: !182, isLocal: false, isDefinition: true, scopeLine: 14, flags: DIFlagPrototyped, isOptimized: false, unit: !2, declaration: !181, variables: !4)
!200 = !DILocalVariable(name: "this", arg: 1, scope: !199, type: !189, flags: DIFlagArtificial | DIFlagObjectPointer)
!201 = !DILocation(line: 0, scope: !199)
!202 = !DILocalVariable(name: "i", arg: 2, scope: !199, file: !3, line: 14, type: !8)
!203 = !DILocation(line: 14, column: 25, scope: !199)
!204 = !DILocation(line: 15, column: 17, scope: !199)
!205 = !DILocation(line: 15, column: 29, scope: !199)
!206 = !DILocation(line: 15, column: 7, scope: !199)
!207 = !DILocation(line: 16, column: 5, scope: !199)
!208 = distinct !DISubprogram(name: "f", linkageName: "_ZN1A1fEPi", scope: !163, file: !3, line: 8, type: !171, isLocal: false, isDefinition: true, scopeLine: 8, flags: DIFlagPrototyped, isOptimized: false, unit: !2, declaration: !170, variables: !4)
!209 = !DILocalVariable(name: "this", arg: 1, scope: !208, type: !162, flags: DIFlagArtificial | DIFlagObjectPointer)
!210 = !DILocation(line: 0, scope: !208)
!211 = !DILocalVariable(name: "i", arg: 2, scope: !208, file: !3, line: 8, type: !8)
!212 = !DILocation(line: 8, column: 25, scope: !208)
!213 = !DILocation(line: 9, column: 16, scope: !208)
!214 = !DILocation(line: 9, column: 28, scope: !208)
!215 = !DILocation(line: 9, column: 7, scope: !208)
!216 = !DILocation(line: 10, column: 5, scope: !208)
