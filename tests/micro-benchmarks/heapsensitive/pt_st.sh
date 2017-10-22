for t in `find ../ -name *.c`;
do 
echo $t;
rm -rf 1.output;
rm -rf 2.output; 
rm -rf pointsto;
opencc -O0 -IPA -OPT:alias=field_sensitive -Wj,-tt24:0x4000 $t
mv points_stat $t.1.output;
opencc -O0 -IPA -OPT:alias=field_sensitive -IPA:icondebug=on -Wj,-tt24:0x4000 $t 
mv points_stat $t.2.output;
diff $t.1.output $t.2.output > $t.diff;

done
