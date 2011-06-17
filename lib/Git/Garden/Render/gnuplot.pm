use strict;
use warnings;

package Git::Garden::Render::gnuplot;

# ABSTRACT: plot a Git::Garden grid with GnuPlot

=head1 SYNOPSIS

    use Git::Garden::Render::gnuplot;
    Git::Garden::Render::gnuplot->plot_grid( $grid );

    firefox simple.png

=cut

use Moose;
use Chart::Gnuplot;
use encoding 'utf8';

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

sub plot_grid {
    my ( $self, $grid ) = @_;

    my $chart = __PACKAGE__->new( file => "simple.png" );
    my $max_col = 0;

    for my $row ( @{$grid} ) {
        next if !$row;

        my $merge_branch_column_index;

        for my $col ( @{ $row->{columns} } ) {
            next if !$col;

            $chart->add( "⎕", $col->{index}, $row->{index} ) if $col->{visuals}{commit};
            $chart->add( "│", $col->{index}, $row->{index} ) if $col->{visuals}{expects_commit};

            $chart->add( "┐", $col->{index}, $row->{index} ) if $col->{visuals}{merge_point};
            $chart->add( "┘", $col->{index}, $row->{index} ) if $col->{visuals}{branch_point};

            $merge_branch_column_index = $col->{index}
              if ( !$merge_branch_column_index or $col->{index} > $merge_branch_column_index ) and $col->{visuals}{merge_point}
              or $col->{visuals}{branch_point};

            $max_col = $col->{index} if $col->{index} > $max_col;
        }

        next if !defined $merge_branch_column_index;

        my $commit_column_index = $row->{commit_column_index};

        for my $connect_index ( $commit_column_index + 1 .. $merge_branch_column_index - 1 ) {
            $chart->add( "─", $connect_index, $row->{index} );
        }
    }

    for my $row ( @{$grid} ) {
        next if !$row;
        $chart->add( $row->{commit}{mini_sha}, $max_col + 5, $row->{index} );
        $chart->add( " ",                      $max_col + 8, $row->{index} );
    }

    $chart->render;

    return;
}

1;
