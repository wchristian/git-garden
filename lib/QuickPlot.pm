use strict;
use warnings;

package QuickPlot;

use Moose;
use Chart::Gnuplot;

has plot     => ( is => 'ro', lazy_build => 1 );
has max_x    => ( is => 'rw', default    => 0 );
has max_y    => ( is => 'rw', default    => 0 );
has file     => ( is => 'rw', required   => 1 );
has commands => ( is => 'rw', default    => sub { "" } );

sub _build_plot {
    my ( $self ) = @_;
    return Chart::Gnuplot->new(
        output   => $self->file . ".png",
        terminal => "png",
        tmargin  => 0,
        bmargin  => 0,
        lmargin  => 0,
        rmargin  => 0,
    );

}

sub add {
    my ( $self, $text, $x, $y ) = @_;

    $self->{commands} .= qq|set label "$text"  at $x, -$y center font "Lucida Console"\n|;

    $self->{max_x} = $x if $x > $self->{max_x};
    $self->{max_y} = $y if $y > $self->{max_y};

    return;
}

sub render {
    my ( $self ) = @_;

    $self->plot->command( $self->commands );

    my @cmds = (
        "set xrange [-1:" . ( $self->max_x + 1 ) . "]",     #
        "set yrange [-" .   ( $self->max_y + 1 ) . ":1]",
        "unset border",
        "unset xtics",
        "unset ytics",
        "set terminal png size " . ( 10 * ( $self->max_x + 1 ) ) . "," . ( 15 * ( $self->max_y + 1 ) ) . "",
    );
    $self->plot->command( $_ ) for @cmds;
    $self->plot->plot2d( Chart::Gnuplot::DataSet->new( xdata => [-2], ydata => [-2], style => "dot" ) );

    return;
}

1;
