#!/usr/bin/perl 

#----------------------------------------------------------------
# Phil Carns
#
# Simple script to generate bmi test data
#
#----------------------------------------------------------------

print `date`;

$reps = 5;
$msg_len = 10000;
$msg_len_jump = 20000;
$msg_len_max = 100000;
$total_len = 10000000;

select STDOUT; $| = 1;

while($msg_len <= $msg_len_max)
{
	$i = 0;
	while($i < $reps)
	{
		print `mpirun -np 16 ./driver_bw_multi -m bmi_tcp -l $msg_len -t $total_len -s 8`;
		$i++;
	}

	$msg_len += $msg_len_jump;
}

print "\n";
		
