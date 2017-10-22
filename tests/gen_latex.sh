#==================================
# Author: Sen Ye (yes@cse.unsw.edu.au)
#
# Collect data from different files with same suffix and merge
# them into a table in a new file.
# All the rows in the source file should have two elements:
# name and data.
# The same row of different source files should begin with same
# name, even if it doesn't have data for that name.
# For example,
# file1       |   file2       |   final file
# row1: 123   |   row1:       |           row1    row2    row3
# row2: 456   |   row2: ade   |   file1   123     456
# row3:       |   row3: 13f   |   file2           ade     13f
# This script doesn't delete any files.
#
# Arguments:
#       $1 suffix of source files
#       $2 folder of source files
#   $3 name of final file
#==================================
suffix=$1;
search_file="*.$suffix";
dir=$2;
file_name=$3;
column_num=0;
process_num=0;

        #the head of tex file
        printf "\\documentclass[a4paper, 12pt]{article}\n" >> $file_name;
        printf "\\usepackage[top=2.5cm,bottom=2.5cm,left=2.5cm,right=2.5cm]{geometry}\n" >> $file_name;
        printf "\\usepackage{lscape}\n" >> $file_name;
        printf "\\" >> $file_name;
        printf "begin{document}\n\n" >> $file_name;
        printf "\\" >> $file_name;
        printf "begin{landscape}\n" >> $file_name;
        printf "\\" >> $file_name;
        printf "begin{table}[htbp]\n" >> $file_name;
        printf "\\centering\n" >> $file_name;
        printf "\\" >> $file_name;
        printf "begin{tabular}{|" >> $file_name;
        cd $dir;
        for i in `find . -name "$search_file" | sort -t '/' -k2,2`;
        do
                process_file=`echo $i | awk -F "/" '{print $NF;}'`;
                if [ $process_num -eq 0 ]
                then
                        #write the style of the tabular
                        awk -F : -v output_file="$file_name" '
                        BEGIN{row=0;}
                        {row++;}
                        END{
                                for(x=0;x<=row;x++){printf("l|") >> output_file;}
                                printf("}\\hline \\hline \n") >> output_file;
                        }
                        ' $i;
                        #write the first line of the tabular
                        awk -F : -v output_file="$file_name" '
                        BEGIN{printf(" ") >> output_file;}
                        {printf(" & %s", $1) >> output_file;}
                        END{printf(" \\\\ \\hline \\hline\n") >> output_file;}
                        ' $i;
                        ((process_num=process_num+1));
                fi
                #write actual data from all the source files
                echo "processing " $process_file;
                spec_name=`echo $process_file | awk -F . '{print $1 "." $2;}'`;
                printf $spec_name >> $file_name;
                awk -F : -v output_file="$file_name" '
                {printf(" & %s",$2) >> output_file;}
                END{
                        printf(" \\\\ \\hline \n") >> output_file;
                }
                ' $i;
        done
        #finish the tex file
        printf "\\" >> $file_name;
        printf "end{tabular}\n" >> $file_name;
        printf "\\" >> $file_name;
        printf "caption{" >> $file_name;
        printf $1 >> $file_name;
        printf "}\n" >> $file_name;
        printf "\\" >> $file_name;
        printf "end{table}\n" >> $file_name;
        printf "\\" >> $file_name;
        printf "end{landscape}\n" >> $file_name;
        printf "\\" >> $file_name;
        printf "end{document}" >> $file_name;
