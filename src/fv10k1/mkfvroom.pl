#!/usr/bin/perl
#     mkfvroom.pl - fv10k1 package
#     This script generates .asm files for Freeverb input patches,
#     for a given room reflection setup (an .rp file)
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
# $Id: mkfvroom.pl,v 1.1 2001/09/28 01:56:20 dbertrand Exp $


use FileHandle;

($infile) = @ARGV;
$infile or die "Please specify input file";

# scan room definition file
warn "Reading $infile\n";
open INFILE,"<$infile" or die "Error opening $infile: $!";
while(<INFILE>)
{
  chomp;
  @f = split;
  $channel = lc $f[0];
  next if $channel !~ /^[rl]$/;
  push @pchan,$channel;
  push @dly,$f[1];
  push @fl,$f[2];
  push @fr,$f[3];
  push @rl,$f[4];
  push @rr,$f[5];
  push @order,$f[6];
  $npaths++;
  $maxdly = $f[1] if $f[1]>$maxdly;
}
close INFILE;
$npaths or die "No paths found";
warn "$npaths reflection paths found\n";

# delay line length - pad with a few extra samples
$dlylength = $maxdly+0.001;
# default reverbration delay is 2/3 of total room reflection
$revdly = $maxdly*0.66; 
# default output delay
$delay_chan = 0.0015;

$now = localtime;

# all output lines
@all_lines = ("fl","fr","rl","rr");
# this hash reverses channels
%revchans = (r=>"l",l=>"r");
# this hash reverses lines
%revlines = (fl=>"fr",fr=>"fl",rl=>"rr",rr=>"rl");

  $revchan = $revchans{$chan};
  $Chan = uc $chan;
# output the file header
  print <<ENDCODE;
;;; FV10K1 room relections code
;;; Generates room reflections according to file $infile, and stores
;;; them in GPRs fvrefl_[fl|fr|rl|rr].
;;; Built on $now from room definition file $infile.

  ; define delays and dampening constants
ENDCODE

# now, add read lines and constants for read lines
# note that right and left paths are symmetric (i.e., paths from R =
# paths from L, but with the corresponding channel inverted)
my $zero = "0";
for( $i=0; $i<$npaths; $i++ )
{
  print "readr${i} tread dlyr,&$dly[$i]\n";
  print "readl${i} tread dlyl,&$dly[$i]\n";
  for ("fl","fr","rl","rr") {
    $x = $_->[$i];
    $x != 0 and print "$_$i constant #$x\n"; 
  }
  # storage for decay filters - N cells, where N=order of path
  print "dmpstore_r${i} sta ",join(",",($zero) x $order[$i]),"\n";
  print "dmpstore_l${i} sta ",join(",",($zero) x $order[$i]),"\n";
}
print "\n";

# output room reflection & accumulation code 
foreach $line (@all_lines)
{
  $Line = uc $line;
  $revline = $revlines{$line};
  $dest = "fvrefl_$line";
  $src = "C_0";
  printf "\n\t;;; Accumulate reflections for $Line\n";
  # Each path has N damping filters attached (where N is the path order).
  # Each filter is of the form:
  #        out[n] = (1-damping)*in + out[n-1]
  #     thus requiring a unit delay, which is kept in GPRs dmpstore_[rl]X+i.
  # This routine prints code to interpolate and updates the delays for a 
  # series of filters, returning a reference to the storage of the last 
  # filter (which is also the output of the series).
  sub Dampening {
    my ($id,$order) = @_;
    my $src = "read$id";
    for( 0..($order[$_]-1) ) {
      my $dest = "dmpstore_$id+$_";
      print "\tinterp $dest,$src,refldamp,$dest\n";
      $src = "$dest";
    }
    return $src;
  }
  # Find terms to be summed for this line.
  # Use paths from channels where the line's coeff is non-0, and
  # reverse channels where the opposite's line coeff is non-0. 
  @terms = ();
  for( 0..($npaths-1) ) 
  {
    my $ch = $pchan[$_];
    $line->[$_] != 0 and push @terms,Dampening("$ch$_",$order[$_]) . ",$line$_";
    $revline->[$_] != 0 and push @terms,Dampening($revchans{$ch}."$_",$order[$_]) . ",$revline$_";
  }
  # Output a MACMV for each term in sum
  if( @terms )
  {
    print "\tmacs C_0,C_0,C_0,C_0   ; reset accumulator\n";
    # write out MACMV for each term up to last one
    while( @terms>1 ) {
      printf "\tmacmv tmp,C_0,%s\n",shift @terms;
    }
    # final term is MACS'd to destination GPR and multiplied by level
    printf "\tmacs $dest,ACCUM,%s\n",shift @terms;
  }
  else
  {
    print ";; no reflections for $dest\n";
    warn "No reflections for line $dest\n";
  }
}

print "\n\tEND\n";
