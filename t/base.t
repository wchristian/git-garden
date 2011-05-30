use strict;
use warnings;

package base_test;

BEGIN {
    chdir '..' if -d '../t';
}

use Test::Most;
use Capture::Tiny::Extended 'capture';
use Test::Regression;

run();
done_testing;

sub run {
    $ENV{TEST_REGRESSION_GEN} = 1;

    test_repo( $_ ) for qw( one cmi );

    return;
}

sub test_repo {
    my ( $repo ) = @_;

    my ( $out, $err, $res ) = capture { system "cd t/repos/$repo && $^X ../../../bin/git-forest --no-color" };
    is $res, 0, "$repo: program exited without a failure value";
    ok_regression sub { $err }, "t/output/$repo\_err", "$repo: stderr matched expected value";
    ok_regression sub { $out }, "t/output/$repo\_out", "$repo: stdout matched expected value";

    return;
}