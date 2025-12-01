#!/bin/bash
function checkSH {
  corename=${1%.*}
  file=test/$corename

  if [ ! -f "$file.sol" ]; then
	echo -en '\e[0;32m'"[Passed! ] "'\e[0;39m'
	echo These is no testcase for the program $1.
  elif [ -f $1 ]; then
	if [ -f $file.arg ]; then
		if [ -f $file.in ]; then
			bash $corename.sh `cat $file.arg` < "$file.in" >& $file.out
		else
			bash $corename.sh `cat $file.arg` >& $file.out
		fi
	elif [ -f $file.in ]; then
		bash $corename.sh < "$file.in" >& $file.out
	else
		bash $corename.sh >& $file.out
	fi

    for xx in `ls $file.arg? 2>/dev/null`
    do
        bash $corename.sh `cat $xx` >> $file.out
    done

    for xx in `ls $file.in? 2>/dev/null`
    do
        bash $corename.sh < $xx >> $file.out
    done


	if diff $file.out $file.sol >/dev/null; then
		echo -en '\e[1;32m'"[Success!]"'\e[0;39m' for $1
		echo
	elif diff $file.out $file.sol2 >& /dev/null; then
		echo -en '\e[1;32m'"[Success!]"'\e[0;39m' for $1
		echo
	else
		echo -en '\e[1;31m'"[Fail!]"'\e[0;39m' for $1
		echo
		echo -en '\e[1;31m'YOUR OUTPUT:'\e[0;39m'
		echo
		cat $file.out | head -15
		echo -en '\e[1;31m'"EXPECTED OUTPUT:"'\e[0;39m'
		echo
		cat $file.sol | head -15
		exit
	fi

  else
	echo File "'"$1"'" does not exist!
  fi
}

function deleteTest {
  corename=${1%.*}
  file=test/$corename
  
  if [ -f $file ]; then
	rm $file
  fi

  if [ -f $file.out ]; then
    rm $file.out
  fi

  if [ $file == "test/ex14" ]; then
    if [ -f $file.sol ]; then
      echo "ERROR: You did not following the instructions" > $file.sol
    fi
  fi
}

if [ -z $1 ]; then
	echo Please type \"./check.sh "<filename>"\".
	echo Other options are \"./check.sh clean\" and \"./check.sh all\".
elif [ $1 = "all" ]; then
	files=`ls *[0-9].sh`
	sorted=`echo $files | awk '{for (ii=NF; ii>=1; ii--) {if (match($ii,"[0-9]+")) 
		{printf substr($ii,RSTART,RLENGTH)" "};print $ii}}' | sort -n | awk '{print $NF}'`
	for xx in $sorted
	do
		checkSH $xx
	done
elif [ $1 = "clean" ]; then
    files=`ls *[0-9].sh`
    sorted=`echo $files | awk '{for (ii=NF; ii>=1; ii--) {if (match($ii,"[0-9]+"))
        {printf substr($ii,RSTART,RLENGTH)" "};print $ii}}' | sort -n | awk '{print $NF}'`
    for xx in $sorted
    do
		deleteTest $xx
    done
else
	checkSH $1
fi
