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

my @header_files = ();
my @dirs = qw();
push(@dirs, $some_dir);

while(@dirs) {
  my $dir = pop(@dirs);
  opendir(my $dh, $dir) || die "Can't open $dir: $!";
  while (readdir $dh) {
      if(-d "$dir/$_" && $_ ne "." && $_ ne "..") {
        push(@dirs, "$dir/$_");
      } elsif($_ =~ m/\.h/) {
        if(not "$dir/$_" =~ /src\/+utils\/+utils.h/) {
          push(@header_files, "$dir/$_");
        }
      }
  }
  closedir $dh;
}

while(@header_files) {
  my $header = pop(@header_files);
  $header =~ s/\/\//\//g;
  open my $fh,"<", "$header" or die $!;
  open my $outf,">", "$header.new" or die $!;
  my $orig_filename = $header;
  my $new_filename = "$header.new";
  $header =~ s/src\///;
  $header =~ s/apps\///;
  $header = uc $header;
  $header =~ s/\//_/g;
  $header =~ s/\./_/g;
  my $define_str = "#define SAF_$header" . "_";
  $header = "#ifndef SAF_$header" . "_";
  my $footer = $header;
  $footer =~ s/ifndef\s/endif  \/\/ /;
  my $count = 0;
  my $current_state = 0;
  while (my $line = <$fh>) {
    if($line =~ /ifndef SAF/) {
      if($line ne "$header\n") {
        print("WARNING: $orig_filename line $count: replacing $line with $header\n");
      }
      print $outf "$header\n";
    } elsif($line =~ /define SAF/) {
      if($line ne "$define_str\n") {
        print("WARNING: $orig_filename line $count: replacing $line with $define_str\n");
      }
      print $outf "$define_str\n";
    } elsif($line =~ /endif.*SAF/) {
      if($line ne "$footer\n") {
        print("WARNING: $orig_filename line $count: replacing $line with $footer\n");
      }
      print $outf "$footer\n";
    } else {
      if($line =~ /ifndef/ || $line =~ /\#if/) {
        $current_state += 1;
        #print "$orig_filename line $count $line";
      }
      if($line =~ /endif/) {
        $current_state -= 1;
        if(not $line =~ /endif\s\s\/\/\s/)  {
          print("WARNING: $orig_filename line $count: possible malformed or missing comment on #endif");
        }
      }
      print $outf $line;
    }
    $count += 1;
  }
  close $fh;
  close $outf;
  if($current_state != 0) {
    print("WARNING: $orig_filename may contain unbalanced #ifs and #endifs");
  }

  rename($new_filename, $orig_filename);
}
