use strict;
use warnings;

package Git::Garden::Render::html;

# ABSTRACT: plot a Git::Garden grid as a single HTML file

=head1 SYNOPSIS

    use Git::Garden::Render::html;
    my $html = Git::Garden::Render::html->plot_grid( $grid );

=cut

use JSON 'to_json';

sub plot_grid {
    my ( $self, $grid ) = @_;

    my @rows;
    for my $row ( @{$grid} ) {
        next if !$row;
        my $uid = $row->{commit}->{uid};
        my $comment = $row->{commit}->{comment};
        push @rows, qq|
            <tr>
                <td id="graph_$uid"></td>
                <td>$uid</td>
                <td class="shorten">$comment</td>
            </tr>
        |;
    }
    my $rows = join "\n", @rows;

    delete $_->{commit}{parents} for @{$grid};
    $_->{commit} = { %{$_->{commit}} } for @{$grid};
    my $json = to_json( $grid );

    my $js = graphlog_js();

    my $html = qq|
<!DOCTYPE html>

<html>
<head>
    <script type="text/javascript" src="http://code.jquery.com/jquery-latest.min.js"></script>
    <script type="text/javascript" src="http://plugins.learningjquery.com/expander/jquery.expander.js"></script>
    <script type="text/javascript" src="http://github.com/DmitryBaranovskiy/raphael/raw/master/raphael.js"></script>
    <script type="text/javascript">
        <!--

            \$(document).ready(
                function() {
                    \$('td.shorten').expander(
                        {
                            slicePoint: 100,
                            widow: 2,
                            expandEffect: 'show',
                            userCollapseText: '[^]'
                        }
                    );
                }
            );

            $js

            var commit_rows = jQuery.makeArray( $json );

            \$(document).ready(function() {
                do_graphs();
            });

        -->
    </script>
</head>

<body>
    <table style="border-collapse:collapse; border-spacing: 0px;">
        <tr>
            <th style="width:200px;">Graph</td>
            <th>Unique ID</th>
            <th>Comment</th>
        </tr>
        $rows
    </table>
</body>
    |;

    return $html;
}

sub graphlog_js {
q|
var colors = Array();
var max_column = 0;
var min_size_limit = 9999;

function do_graphs() {
    fill_colors();
    var graphs = Array();
    $(commit_rows).each(function () {
        if ( !this.columns ) return;
        if ( this.columns.length > max_column ) max_column = this.columns.length;
        return;
    });

    $(commit_rows).each(function () {
        var r = render_canvas( 'graph_' + this.commit.uid );
        if ( r ) this.canvas = r;
        return;
    });

    $(commit_rows).each(function () {
        var commit_row = this;

        if ( !commit_row.canvas ) return;

        var canvas = commit_row.canvas;
        var commit_column_index = commit_row.commit_column_index;

        $(commit_row.columns).each(function () {
            var column = this;
            if ( !column ) return;
            if ( !column.visuals ) return;

            if ( column.visuals.commit         ) draw_commit(       canvas, column.index);
            if ( column.visuals.expects_commit ) draw_expects(      canvas, column.index);
            if ( column.visuals.merge_point    ) draw_merge_point(  canvas, column.index);
            if ( column.visuals.branch_point   ) draw_branch_point( canvas, column.index);

            if ( column.visuals.merge_point    ) draw_branch_merge_line( canvas, column.index, column.index - commit_column_index - 1 );
            if ( column.visuals.branch_point   ) draw_branch_merge_line( canvas, column.index, column.index - commit_column_index - 1 );

            return;
        });

        return;
    });

    var graph_index = graphs.length;
    while (graph_index--) {
        var graph = graphs[graph_index];
        var color_index = max_column + 1;
        while (color_index--) {
            draw_expects(graph, color_index);
            draw_commit(graph, color_index);
            draw_merge_point(graph, color_index);
            draw_branch_point(graph, color_index);
        }
    }
}

function draw_expects(r, column) {
    var cell = get_graph_cell(r, column);

    var path = r.path("M0 0L0 {0}", r.height);
    transform_to_cell(path, cell);

    return;
}

function draw_commit(r, column) {
    var cell = get_graph_cell(r, column);

    var circle = r.circle(0, r.height / 2, min_size_limit / 2 * .7);
    transform_to_cell(circle, cell);

    return;
}

function draw_merge_point(r, column) {
    var cell = get_graph_cell(r, column);

    var center_offset = min_size_limit / 3;
    var merge_point = r.path(
        "M{0} {1}L{2} {3}L{4} {5}L{6} {7}", //
        -1 * cell.width / 2,r.height / 2, //
        -1 * center_offset, r.height / 2, //
        0,                  r.height / 2 + center_offset, //
        0,                  r.height //
    );
    transform_to_cell(merge_point, cell);

    return;
}

function draw_branch_point(r, column) {
    var cell = get_graph_cell(r, column);

    var center_offset = min_size_limit / 3;
    var branch_point = r.path(
        "M{0} {1}L{2} {3}L{4} {5}L{6} {7}", //
        0,                  0, //
        0,                  r.height / 2 - center_offset, //
        -1 * center_offset, r.height / 2, //
        -1 * cell.width / 2,r.height / 2 //
    );
    transform_to_cell(branch_point, cell);

    return;
}

function draw_branch_merge_line(r, column, line_length) {
    if ( !line_length ) return;
    var cell = get_graph_cell(r, column);

    var center_offset = min_size_limit / 3;
    var branch_merge_line = r.path(
        "M{0} {1}L{2} {3}", //
        0 - cell.width * line_length, r.height / 2, //
        0,                            r.height / 2
    );
    branch_merge_line.translate(cell.width / -2, 0);

    transform_to_cell(branch_merge_line, cell);

    return;
}

function transform_to_cell(vector_obj, cell) {
    vector_obj.translate(cell.width / 2, 0);
    vector_obj.translate(cell.offset, 0);
    vector_obj.attr({
        stroke: colors[cell.column % 8]
    });

    return;
}

function get_graph_cell(r, column) {
    var width = r.width / (max_column + 1);
    var offset = width * column;
    var size_limit = width;
    if ( r.height < size_limit ) size_limit = r.height;
    return {
        size_limit: size_limit,
        width: width,
        offset: offset,
        column: column
    };
}

function fill_colors() {
    colors[0] = "#000000";
    var color_index = 7;
    while (color_index--) {
        Raphael.getColor();
        colors[color_index + 1] = Raphael.getColor();
    }
    return;
}

function render_canvas(id) {
    var target = $('#' + id);
    if ( !target.length ) return;
    var x = target.offset().left;
    var y = target.offset().top;
    var width = target.innerWidth();
    var height = target.innerHeight();

    target.css('padding', '0');
    target.css('line-height', '0');

    var r = Raphael(id, width, height);

    if ( r.height < min_size_limit ) min_size_limit = r.height;
    if ( r.width / max_column + 1 < min_size_limit ) min_size_limit = r.width / max_column + 1;

    return r;
}
|;
}

1;
