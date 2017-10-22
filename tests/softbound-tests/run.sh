rm -rf *.dvf.bc
for i in `ls *.bc`
do
echo process $i;
#dvf -debug $i;
#dvf -debug-only=dumppt $i;
dvf $1 -print-pts $i;
done
rm -rf *.dvf.bc
