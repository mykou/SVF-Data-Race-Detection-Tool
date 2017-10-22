for i in `find . -name 'Debug*'`
do
echo delete $i
rm -rf $i
done
for i in `find . -name 'Release*'`
do
echo delete $i
rm -rf $i
done
for i in `find ./lib/RuntimeLib -name '*.o'`
do
echo delete $i
rm -rf $i
done
