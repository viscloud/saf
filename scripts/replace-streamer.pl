# Copyright 2018 The SAF Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

use 5.012;
my $some_dir = $ARGV[0];

my @files = ();
my @dirs = qw();
push(@dirs, $some_dir);

while(@dirs) {
  my $dir = pop(@dirs);
  opendir(my $dh, $dir) || die "Can't open $dir: $!";
  while (readdir $dh) {
      if(-d "$dir/$_" && $_ ne "." && $_ ne "..") {
        push(@dirs, "$dir/$_");
      } elsif($_ ne "." && $_ ne "..") {
        push(@files, "$dir/$_");
      }
  }
  closedir $dh;
}

sub get_user_input {
  my $c1 = $_[0];
  my $c2 = $_[1];
  print("1: $c1");
  print("2: $c2");
  my $choice = <STDIN>;
  chomp $choice;
  if($choice eq "" || $choice eq "1") {
    return $c1;
  } elsif($choice eq "2") {
    return $c2;
  } else {
    print("INVALID");
    return "";
  }
}

while(@files) {
  my $f = pop(@files);
  print "$f\n";
  open my $fh,"<", "$f" or die $!;
  open my $outf,">", "$f.new" or die $!;
  while (my $line = <$fh>) {
	  print("$line >> ");
    while($line =~ /Streamer/) {
      $line =~ s/Streamer/"Saf"/e;
    }
    while($line =~ /STREAMER/) {
      $line =~ s/STREAMER/"SAF"/e;
    }
    while($line =~ /streamerpy/) {
      $line =~ s/streamerpy/" safpy"/e;
    }
    while($line =~ /streamer_status/) {
      $line =~ s/streamer_status/"saf_status"/e;
    }
    while($line =~ /GSaf/) {
      $line =~ s/GSaf/"GStreamer"/e
    }
    while($line =~ /GSAF/) {
      $line =~ s/GSAF/"GSTREAMER"/e
    }
    while($line =~ /Saf/) {
      $line =~ s/Saf/"SAF"/e
    }
    print("$line\n");
    print $outf $line
  }
  rename("$f.new", $f);
}
