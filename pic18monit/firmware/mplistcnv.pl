#!/usr/bin/perl

# ‚Ps‚ÌÅ‘å’·.
my $maxlength=79;

my $curfile='';

sub usage()
{
	print <<'EOH';
# Usage:
#   $ mplistcnv.pl infile.lst

EOH
	exit 1;
}

# =====================================================
#      
# =====================================================
sub loadfile()
{
	my ($file)=@_;
	my @buf;
	open(FILE,$file) || die("Can't open file $file");
	@buf = <FILE>;
	close(FILE);
	return @buf;
}

sub setfile()
{
	my ($file)=@_;
	if( $curfile ne $file) {
		$curfile = $file;
		printf("FILE:%s\n",$file);
	}
}

sub cutline()
{
	my ($line)=@_;
	my $n = rindex($line,' ');
	my $s = substr($line,$n+1);
	$line = substr($line,0,$n);

	if($s =~ /^[C-Z]:/ ) {
		&setfile($s);
		$line =~ s/[ ]+$//;
	}
	$line = substr($line,0,$maxlength);
	$line =~ s/[ ]+$//;
	return $line;
}

sub pr()
{
	my ($line)=@_;
	$line = &cutline($line);
	print $line . "\n";
}

sub main()
{
	my $line;
	if(scalar(@ARGV)<1) {
		&usage();
	}
	my @buf = &loadfile($ARGV[0]);
	foreach $line(@buf) {
		$line =~ s/\n//;
		$line =~ s/\r//;
		&pr($line);
	}
}


&main();

