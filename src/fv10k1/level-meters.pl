#!/usr/bin/perl

# hash of parameter names

%levnumber = ( maxlev_fr => 0, maxlev_fl => 1,
               maxlev_rr => 2, maxlev_rl => 3 );  

for(;;)
{
  my @lines = `emu-dspmgr -c`;
  my $arg = "none";
  %values = ();
  for( @lines )
  {
    /name: +([\w]+)/ and do { $arg = $1; };
    /value: +([\w]+)/ and defined $levnumber{$arg} and 
      $level[$levnumber{$arg}] = $1;
  }
  print join " ",@level,"\n";
}  
