/*
void update_num_countdown(int x, int y, short int line_color, int num) {
    if (num==4 || num==1) {         // erase seg 0
        for (int r=y+2; r<y+4; r++)
        for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, colour);
    } else if (num==3 || num==0) {  // draw seg 0
        for (int r=y+2; r<y+4; r++)
        for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, line_color);
    }
    if (num==6) {           // erase seg 1
        for (int r=y+4; r<y+19; r++)
        for (int c=x+num_w-8; c<x+num_w-6; c++)
        plot_pixel(c, r, colour);
    } else if (num==4) {    // draw seg 1
        for (int r=y+4; r<y+19; r++)
        for (int c=x+num_w-8; c<x+num_w-6; c++)
        plot_pixel(c, r, line_color);
    }
    if (num==2) {           // erase seg 2
        for (int r=y+21; r<y+num_l-4; r++)
        for (int c=x+num_w-8; c<x+num_w-6; c++)
        plot_pixel(c, r, colour);
    } else if (num==1) {    // draw seg 2
        for (int r=y+21; r<y+num_l-4; r++)
        for (int c=x+num_w-8; c<x+num_w-6; c++)
        plot_pixel(c, r, line_color);
    }
    if (num==7 || num==1) {         // erase seg 3
        for (int r=y+36; r<y+num_l-2; r++)
        for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, colour);
    } else if (num==6 || num==0) {  // draw seg 3
        for (int r=y+36; r<y+num_l-2; r++)
        for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, line_color);
    }
    if (num==9 || num==7 || num==5 || num==1) {         // erase seg 4
        for (int r=y+21; r<y+num_l-4; r++)
        for (int c=x+6; c<x+8; c++)
        plot_pixel(c, r, colour);
    } else if (num==8 || num==6 || num==2 || num==0) {  // draw seg 4
        for (int r=y+21; r<y+num_l-4; r++)
        for (int c=x+6; c<x+8; c++)
        plot_pixel(c, r, line_color);
    }
    if (num==7 || num==3) {         // erase seg 5
        for (int r=y+4; r<y+19; r++)
        for (int c=x+6; c<x+8; c++)
        plot_pixel(c, r, colour);
    } else if (num==6 || num==0) {  // draw seg 5
        for (int r=y+4; r<y+19; r++)
        for (int c=x+6; c<x+8; c++)
        plot_pixel(c, r, line_color);
    }
    if (num==7 || num==1) {         // erase seg 6
        for (int r=y+19; r<y+21; r++)
        for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, colour);
    } else if (num==9 || num==6) {  // draw seg 6
        for (int r=y+19; r<y+21; r++)
        for (int c=x+8; c<x+num_w-8; c++)
        plot_pixel(c, r, line_color);
    }
}
    */