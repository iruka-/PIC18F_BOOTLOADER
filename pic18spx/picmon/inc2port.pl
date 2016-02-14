#!/usr/bin/perl

my %hash;

my @labels;

sub main()
{
	while(<>) {
		$line = $_;
		$line =~ s/\r//;
		$line =~ s/\n//;
		my ($label,$value)=split(/EQU/,$line);
		$label =~ s/ //g;
		$value =~ s/ //g;
		$value =~ s/H'//;
		$value =~ s/'//;
		$value = '0x' . $value;

#		print "$label , $value \n";
		$hash{$value} = $label;
	}

	foreach $line(keys %hash) {
		$value = $line;
		$label = '"' . $hash{$value} . '"';
#		printf("	{%-16s,%s,0	},\n",$label , $value );
		push @labels,"$value,$label";
	}

	my @label = sort(@labels);
	foreach $line(@label) {
		($value,$label)=split(/,/,$line);
		printf("	{%-16s,%s,0	},\n",$label , $value );
	}

}


&main();

