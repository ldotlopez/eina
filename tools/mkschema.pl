#!/usr/bin/perl

use strict;

my $chunk = '  <schema>
    <key>/schemas/apps/eina-gnome/%s</key>
    <applyto>/apps/eina-gnome/%s</applyto>
    <owner>eina-gnome</owner>
    <type>%s</type>
    <default>%s</default>
    <locale name="C">
      <short></short>
      <long></long>
    </locale>
  </schema>';
		
my @schemas;
while ( @ARGV ) {
	my ($path, $type, $def);
	$path = shift;
	$type = shift;
	$def  = shift;

	push @schemas, sprintf($chunk, $path, $path, $type, $def);
}

print join "\n\n", @schemas;
