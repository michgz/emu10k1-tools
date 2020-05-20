#!/usr/bin/perl
#     fv10k1control.pl - fv10k1 package
#     Interactive control of fv10k1 parameters
#         
#     Copyright (C) 2001 Oleg Smirnov <smirnov@astron.nl>
#    
#     This program is free software; you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation; either version 2 of the License, or
#     (at your option) any later version.
# 
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public License
#     along with this program; if not, write to the Free Software
#     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# $Id: fv10k1control.pl,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

# hash of parameter names
  
$maxhex = 0xffffffff;
$maxpos = 0x7fffffff;

@allparms = ( 
  "DL","delay_l",
  "DR","delay_r",
  "DRV","revdelay",
  "RVG","revgain",
  "RSZ","roomsize",
  "DMP","damp",
  "APF","allpassfeed",
  "WET","wet1",
  "DRY","dry",
  "RVB","revbass",
  "RVT","revtreble",
  "RVD","revdefeat",
  "RFD","refldamp",
  "FRV","reverb_f",
  "FRF","refl_f",
  "FLV","ptf_level",
  "RRV","reverb_r",
  "RRG","reverb_rgain",
  "RRF","refl_r",
  "RFG","refl_rgain",
  "RLV","ptr_level",
  "RGA","ptr_gain",
  "RBA","ptr_bass",
  "RTR","ptr_treble",
  "RTD","ptr_defeat",
   );

while( @allparms ) {
  $name = shift @allparms;
  $parms{$name} = shift @allparms;
  push @parmlist,$name;
}

%values = ();

sub tofloat  {
  my ($t) = @_;
  $t > $maxpos and $t = $maxhex-$t+1;
  return $t/$maxpos;
}
  
sub read_val {
  my @lines = `emu-dspmgr -c`;
  my $arg = "none";
  %values = ();
  for( @lines )
  {
    /name: +([\w]+)/ and do { $arg = $1; };
    /value: +([\w]+)/ and do { $values{$arg} = hex $1; }; 
  }
  for( values %parms ) {
    defined $values{$_} or die "Unknown parameter: $_";
  }
}  

sub print_val {
  print "\n";
  for( @parmlist ) 
  {
    my $arg = $parms{$_};
    printf "%3s %-12s 0x%-8x #%4.2f &%.2f\n",$_,$arg,
         $values{$arg},
         tofloat($values{$arg}),
         $values{$arg}/(48000.0*0x800);
  }
}

read_val;
print_val;
print ">> ";

while(<>) 
{
  for( split /;/ )
  {
    my $ok = 0;
    /([^\s]+) +([#&\d.xa-f]+)/ and do {
      my $id = uc $1;
      my $value = $2;
      my $name = $parms{$id};
      if( $name )
      {
        printf "Setting %s (%s) to %s\n",$id,$name,$value;
        $value =~ /^[\d]*\.[\d]*/ and $value = "#$value";
        $value =~ /^(#|&)/ and $value = "\\$value";
        my $cmd = "emu-dspmgr -p Freeverb -c$name -s $value";
        print "$cmd\n";
        `$cmd`;
        $ok = 1;
        read_val;
        print_val;
      }
    };
    $ok or print "error in input $id\n";
  }
  print ">> ";
}


