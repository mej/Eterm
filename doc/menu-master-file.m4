<Eterm-0.9.1>

dnl# These macros are used everywhere.
define(STRING_ITEM, `begin menuitem
    text "$1"
    ifelse($3, `', `action string "$2"', `rtext "$2"
    action string "$3"')
  end')
define(ECHO_ITEM, `begin menuitem
    text "$1"
    ifelse($3, `', `action echo "$2"', `rtext "$2"
    action echo "$3"')
  end')
define(SUBMENU_ITEM, `begin menuitem
    text "$1"
    action submenu "ifelse($2, `', $1, $2)"
  end')
define(SCRIPT_ITEM, `begin menuitem
    text "$1"
    ifelse($3, `', `action script "$2"', `rtext "$2"
    action script "$3"')
  end')
define(FONT_ITEM, `begin menuitem
    text "$1"
    action string "\e]50;#$2"
  end')
define(TILE_IMAGE_ITEM, `begin menuitem
    text "$1"
    action string "\e]20;tile/$2;0+0:tile\a"
  end')
define(SCALE_IMAGE_ITEM, `begin menuitem
    text "$1"
    action string "\e]20;scale/$2;100\a"
  end')


dnl#
dnl# Define these only for the chooser theme
dnl#
ifdef(`CHOOSER', `
dnl# HOST_ITEM(protocol, hostname [, ip])
define(HOST_ITEM, `begin menuitem
    text "$2"
    action script "string(\e]0\\;$2\\ $1\a); echo($1\\ '`ifelse($3, `', `$2', `$3')'`\r);"
  end')
dnl# INT_MENU(protocol)
define(INT_MENU, `begin menu
  title "Internal Hosts $1"
  HOST_ITEM(`$1', `alpha')
  HOST_ITEM(`$1', `beta')
  HOST_ITEM(`$1', `gamma', `10.1.1.10')
  HOST_ITEM(`$1', `delta')
end')
define(EXT_MENU, `begin menu
  title "External Hosts $1"
  HOST_ITEM(`$1', `carbon.foo.com')
  HOST_ITEM(`$1', `i.am.danish.so.eatme.net')
  HOST_ITEM(`$1', `oxygen.bar.org')
end')

define(PROTOCOL_MENU, `begin menu
  title "$1"
  begin menuitem
    text "Internal Hosts"
    action submenu "Internal Hosts $1"
  end
  begin menuitem
    text "External Hosts"
    action submenu "External Hosts $1"
  end
end')
define(DO_PROTOCOL, `INT_MENU($1)
EXT_MENU($1)
PROTOCOL_MENU($1)')
')

dnl#
dnl# Beginning of the actual menus
dnl#
begin menu
  title Font
  FONT_ITEM(`Normal', `')
  FONT_ITEM(`Larger', `+')
  FONT_ITEM(`Smaller', `-')
  -
  FONT_ITEM(`Font 1', `1')
  FONT_ITEM(`Font 2', `2')
  FONT_ITEM(`Font 3', `3')
  FONT_ITEM(`Font 4', `4')
end
begin menu
  title Toggles
  STRING_ITEM(`Cursor Visible', `\e[?25t')
  STRING_ITEM(`Home on Output', `\e[?1010t')
  STRING_ITEM(`Home on Input', `\e[?1012t')
  STRING_ITEM(`Map Alert', `\e]6;21\a')
  STRING_ITEM(`Primary/Secondary Screen', `\e[?47t')
  STRING_ITEM(`Reverse Video', `\e[?5t')
  STRING_ITEM(`Select Trailing Spaces', `\e]6;25\a')
  STRING_ITEM(`Select Whole Line on Triple-click', `\e]6;23\a')
  STRING_ITEM(`Visual Bell', `\e]6;20\a')
  STRING_ITEM(`X11 Mouse Reporting', `\e[?1000t')
  STRING_ITEM(`Xterm Selection Behavior', `\e]6;22\a')
end
begin menu
  title Keys
  STRING_ITEM(`Application Arrow Keys', `\e[?1t')
  STRING_ITEM(`Application Keypad', `\e[?66t')
  STRING_ITEM(`Backspace/Delete', `\e[?67t')
  STRING_ITEM(`Xterm Shift-Keys', `\e[?35t')
end
begin menu
  title Scrollbar
  STRING_ITEM(`Toggle Visibility', `Ctrl-Button2', `\e[?30t')
  STRING_ITEM(`Swap Sides', `\e]6;11\a')
  STRING_ITEM(`Toggle Floating', `\e]6;12\a')
  STRING_ITEM(`Toggle Popup Mode', `\e]6;13\a')
  -
  STRING_ITEM(`Motif (Traditional) Style', `\e]6;10;motif;;\a')
  STRING_ITEM(`NeXT Style', `\e]6;10;next;;\a')
  STRING_ITEM(`Xterm Style', `\e]6;10;xterm;;\a')
end
begin menu
  title Width
  STRING_ITEM(`5', `\e]6;10;;5;\a')
  STRING_ITEM(`6', `\e]6;10;;6;\a')
  STRING_ITEM(`7', `\e]6;10;;7;\a')
  STRING_ITEM(`8', `\e]6;10;;8;\a')
  STRING_ITEM(`9', `\e]6;10;;9;\a')
  STRING_ITEM(`10', `\e]6;10;;10;\a')
  STRING_ITEM(`11', `\e]6;10;;11;\a')
  STRING_ITEM(`12', `\e]6;10;;12;\a')
  STRING_ITEM(`13', `\e]6;10;;13;\a')
  STRING_ITEM(`14', `\e]6;10;;14;\a')
  STRING_ITEM(`15', `\e]6;10;;15;\a')
  STRING_ITEM(`16', `\e]6;10;;16;\a')
  STRING_ITEM(`17', `\e]6;10;;17;\a')
  STRING_ITEM(`18', `\e]6;10;;18;\a')
  STRING_ITEM(`19', `\e]6;10;;19;\a')
  STRING_ITEM(`20', `\e]6;10;;20;\a')
end
begin menu
  title "Move to Desktop"
  STRING_ITEM(`0', `\e]6;50;0;\a')
  STRING_ITEM(`1', `\e]6;50;1;\a')
  STRING_ITEM(`2', `\e]6;50;2;\a')
  STRING_ITEM(`3', `\e]6;50;3;\a')
  STRING_ITEM(`4', `\e]6;50;4;\a')
  STRING_ITEM(`5', `\e]6;50;5;\a')
  STRING_ITEM(`6', `\e]6;50;6;\a')
  STRING_ITEM(`7', `\e]6;50;7;\a')
end
begin menu
  title Terminal
  SUBMENU_ITEM(`Toggles')
  SUBMENU_ITEM(`Keys')
  SUBMENU_ITEM(`Scrollbar')
  SUBMENU_ITEM(`Move to Desktop')
  -
  STRING_ITEM(`Steal Focus', `Ctrl-Button1', `\e]5;\a')
  STRING_ITEM(`Full Reset', `\ec')
end

begin menu
  title Tiled
  TILE_IMAGE_ITEM(`Acid Chess', `acid_chess.png')
  TILE_IMAGE_ITEM(`A Distant Star', `a_distant_star.png')
  TILE_IMAGE_ITEM(`Alpha Centauri', `alpha_centauri.png')
  TILE_IMAGE_ITEM(`Army Brat', `army_brat.png')
  TILE_IMAGE_ITEM(`Bubbly1', `bubbly1.jpg')
  TILE_IMAGE_ITEM(`Bubbly2', `bubbly2.jpg')
  TILE_IMAGE_ITEM(`Bubbly3', `bubbly3.jpg')
  TILE_IMAGE_ITEM(`Bubbly4', `bubbly4.jpg')
  TILE_IMAGE_ITEM(`Circuit', `circuit.jpg')
  TILE_IMAGE_ITEM(`Cold Steel', `cold_steel.png')
  TILE_IMAGE_ITEM(`Conch', `conch.png')
  TILE_IMAGE_ITEM(`Confusion', `confusion.jpg')
  TILE_IMAGE_ITEM(`Connect The Dots', `connect_the_dots.png')
  TILE_IMAGE_ITEM(`Cyber Bg', `cyber_bg.jpg')
  TILE_IMAGE_ITEM(`Darkness', `darkness.png')
  TILE_IMAGE_ITEM(`Falling', `falling.png')
  TILE_IMAGE_ITEM(`Fascination', `fascination.png')
  TILE_IMAGE_ITEM(`Firestorm', `firestorm.png')
  TILE_IMAGE_ITEM(`For A Moment', `for_a_moment.png')
  TILE_IMAGE_ITEM(`Graffiti', `graffiti.png')
  TILE_IMAGE_ITEM(`Gray Matter', `gray_matter.png')
  TILE_IMAGE_ITEM(`Lightning Crashes', `lightning_crashes.png')
  TILE_IMAGE_ITEM(`Marbles', `marbles.png')
  TILE_IMAGE_ITEM(`Moody Blues', `moody_blues.png')
  TILE_IMAGE_ITEM(`Murky Depths', `murky_depths.png')
  TILE_IMAGE_ITEM(`Oil On Canvas', `oil_on_canvas.png')
  TILE_IMAGE_ITEM(`Pebble Beach', `pebble_beach.png')
  TILE_IMAGE_ITEM(`Ping Pong', `ping-pong.png')
  TILE_IMAGE_ITEM(`Plasma', `plasma.png')
  TILE_IMAGE_ITEM(`Pulse', `pulse.png')
  TILE_IMAGE_ITEM(`Rolling Meadows', `rolling_meadows.png')
  TILE_IMAGE_ITEM(`Shades Of Being', `shades_of_being.png')
  TILE_IMAGE_ITEM(`Shale', `shale.jpg')
  TILE_IMAGE_ITEM(`Shattered Windows', `shattered_windows.png')
  TILE_IMAGE_ITEM(`Shock And Awe', `shock_and_awe.png')
  TILE_IMAGE_ITEM(`Sitting In Acid', `sitting_in_acid.png')
  TILE_IMAGE_ITEM(`Spring Rain', `spring_rain.png')
  TILE_IMAGE_ITEM(`Stained Glass', `stained_glass.png')
  TILE_IMAGE_ITEM(`Tales Of Earthsea', `tales_of_earthsea.png')
  TILE_IMAGE_ITEM(`Tangled Web', `tangled_web.png')
  TILE_IMAGE_ITEM(`The Wall', `the_wall.png')
  TILE_IMAGE_ITEM(`Trees Are Leafy', `trees_are_leafy.png')
  TILE_IMAGE_ITEM(`Veins 2', `veins_2.png')
  TILE_IMAGE_ITEM(`Veins', `veins.png')
  TILE_IMAGE_ITEM(`Violetta', `violetta.png')
  TILE_IMAGE_ITEM(`Water Into Wine', `water_into_wine.png')
  TILE_IMAGE_ITEM(`Wormhole', `wormhole.png')
  TILE_IMAGE_ITEM(`Woven Memories', `woven_memories.png')
end
begin menu
  title Scaled
  SCALE_IMAGE_ITEM(`Day Night', `day-night.jpg')
  SCALE_IMAGE_ITEM(`Midnight Sky', `midnight_sky.png')
  SCALE_IMAGE_ITEM(`Neopolis Horizon', `Neopolis-horizon.jpg')
  SCALE_IMAGE_ITEM(`Sailors Delight', `sailors_delight.png')
  SCALE_IMAGE_ITEM(`Wasteland', `wasteland.jpg')
  SCALE_IMAGE_ITEM(`Wintertree', `wintertree.jpg')
end
begin menu
  title Attrib
  STRING_ITEM(`Full Size', `\e]20;;100x100')
  STRING_ITEM(`Normal Size', `\e]20;;0x0')
  -
  STRING_ITEM(`Tiled', `\e]20;;:tile')
  STRING_ITEM(`Scaled', `\e]20;;100x100')
  STRING_ITEM(`Proportional Scale', `\e]20;;100x100:propscale')
  -
  STRING_ITEM(`Center', `\e]20;;=+50+50')
  STRING_ITEM(`Top Left Corner', `\e]20;;=+0+0')
  STRING_ITEM(`Bottom Right Corner', `\e]20;;=+100+100')
end
begin menu
  title Pixmap
  STRING_ITEM(`None', `\e[m\e]20;;\a')
  STRING_ITEM(`Toggle Viewport Mode', `\e]6;24\a')
  SUBMENU_ITEM(`Tiled')
  SUBMENU_ITEM(`Scaled')
  -
  SUBMENU_ITEM(`Attrib')
end
begin menu
  title Brightness
  STRING_ITEM(`512 (200\%)', `\e]6;1;image;brightness;512\a')
  STRING_ITEM(`480 (188\%)', `\e]6;1;image;brightness;480\a')
  STRING_ITEM(`448 (175\%)', `\e]6;1;image;brightness;448\a')
  STRING_ITEM(`416 (163\%)', `\e]6;1;image;brightness;416\a')
  STRING_ITEM(`384 (150\%)', `\e]6;1;image;brightness;384\a')
  STRING_ITEM(`352 (138\%)', `\e]6;1;image;brightness;352\a')
  STRING_ITEM(`320 (125\%)', `\e]6;1;image;brightness;320\a')
  STRING_ITEM(`288 (113\%)', `\e]6;1;image;brightness;288\a')
  STRING_ITEM(`256 (100\%)', `\e]6;1;image;brightness;256\a')
  STRING_ITEM(`224 (88\%)', `\e]6;1;image;brightness;224\a')
  STRING_ITEM(`192 (75\%)', `\e]6;1;image;brightness;192\a')
  STRING_ITEM(`160 (63\%)', `\e]6;1;image;brightness;160\a')
  STRING_ITEM(`128 (50\%)', `\e]6;1;image;brightness;128\a')
  STRING_ITEM(`96 (38\%)', `\e]6;1;image;brightness;96\a')
  STRING_ITEM(`64 (25\%)', `\e]6;1;image;brightness;64\a')
  STRING_ITEM(`32 (13\%)', `\e]6;1;image;brightness;32\a')
  STRING_ITEM(`0 (0\%)', `\e]6;1;image;brightness;0\a')
end
begin menu
  title Contrast
  STRING_ITEM(`512 (200\%)', `\e]6;1;image;contrast;512\a')
  STRING_ITEM(`480 (188\%)', `\e]6;1;image;contrast;480\a')
  STRING_ITEM(`448 (175\%)', `\e]6;1;image;contrast;448\a')
  STRING_ITEM(`416 (163\%)', `\e]6;1;image;contrast;416\a')
  STRING_ITEM(`384 (150\%)', `\e]6;1;image;contrast;384\a')
  STRING_ITEM(`352 (138\%)', `\e]6;1;image;contrast;352\a')
  STRING_ITEM(`320 (125\%)', `\e]6;1;image;contrast;320\a')
  STRING_ITEM(`288 (113\%)', `\e]6;1;image;contrast;288\a')
  STRING_ITEM(`256 (100\%)', `\e]6;1;image;contrast;256\a')
  STRING_ITEM(`224 (88\%)', `\e]6;1;image;contrast;224\a')
  STRING_ITEM(`192 (75\%)', `\e]6;1;image;contrast;192\a')
  STRING_ITEM(`160 (63\%)', `\e]6;1;image;contrast;160\a')
  STRING_ITEM(`128 (50\%)', `\e]6;1;image;contrast;128\a')
  STRING_ITEM(`96 (38\%)', `\e]6;1;image;contrast;96\a')
  STRING_ITEM(`64 (25\%)', `\e]6;1;image;contrast;64\a')
  STRING_ITEM(`32 (13\%)', `\e]6;1;image;contrast;32\a')
  STRING_ITEM(`0 (0\%)', `\e]6;1;image;contrast;0\a')
end
begin menu
  title Gamma
  STRING_ITEM(`512 (200\%)', `\e]6;1;image;gamma;512\a')
  STRING_ITEM(`480 (188\%)', `\e]6;1;image;gamma;480\a')
  STRING_ITEM(`448 (175\%)', `\e]6;1;image;gamma;448\a')
  STRING_ITEM(`416 (163\%)', `\e]6;1;image;gamma;416\a')
  STRING_ITEM(`384 (150\%)', `\e]6;1;image;gamma;384\a')
  STRING_ITEM(`352 (138\%)', `\e]6;1;image;gamma;352\a')
  STRING_ITEM(`320 (125\%)', `\e]6;1;image;gamma;320\a')
  STRING_ITEM(`288 (113\%)', `\e]6;1;image;gamma;288\a')
  STRING_ITEM(`256 (100\%)', `\e]6;1;image;gamma;256\a')
  STRING_ITEM(`224 (88\%)', `\e]6;1;image;gamma;224\a')
  STRING_ITEM(`192 (75\%)', `\e]6;1;image;gamma;192\a')
  STRING_ITEM(`160 (63\%)', `\e]6;1;image;gamma;160\a')
  STRING_ITEM(`128 (50\%)', `\e]6;1;image;gamma;128\a')
  STRING_ITEM(`96 (38\%)', `\e]6;1;image;gamma;96\a')
  STRING_ITEM(`64 (25\%)', `\e]6;1;image;gamma;64\a')
  STRING_ITEM(`32 (13\%)', `\e]6;1;image;gamma;32\a')
  STRING_ITEM(`0 (0\%)', `\e]6;1;image;gamma;0\a')
end
begin menu
  title Background
  STRING_ITEM(`Toggle Transparency', `\e]6;0\a')
  SUBMENU_ITEM(`Pixmap')
  SUBMENU_ITEM(`Brightness')
  SUBMENU_ITEM(`Contrast')
  SUBMENU_ITEM(`Gamma')
end

dnl# chooser-only menus
ifdef(`CHOOSER', `
DO_PROTOCOL(ssh)
DO_PROTOCOL(rlogin)
DO_PROTOCOL(telnet)
DO_PROTOCOL(ftp)
')

dnl### BEGIN mutt menus
ifdef(`MUTT', `
begin menu
  title Messages
  ECHO_ITEM(`New Message', `m')
  ECHO_ITEM(`Reply', `r')
  ECHO_ITEM(`Group Reply', `g')
  ECHO_ITEM(`Forward', `f')
  ECHO_ITEM(`Bounce', `b')
  ECHO_ITEM(`Move Message to...', `s')
  ECHO_ITEM(`Copy Message to...', `C')
  ECHO_ITEM(`Delete Message', `d')
  ECHO_ITEM(`Display Message', `\r')
  ECHO_ITEM(`Toggle Headers', `h')
  ECHO_ITEM(`Show Attachments', `v')
  -
  ECHO_ITEM(`Exit mutt', `q')
  ECHO_ITEM(`Exit without Saving', `x')
end

begin menu
  title "Change Folder"
  ECHO_ITEM(`Inbox', `c!\r')
  ECHO_ITEM(`Sent Mail', `c<\r')
  ECHO_ITEM(`Received Mail', `c>\r')
  ECHO_ITEM(`Other...', `c')
end

begin menu
  title Forward
  ECHO_ITEM(`Forward by Message Date', `od')
  ECHO_ITEM(`Forward by Sender', `of')
  ECHO_ITEM(`Forward by Date Received', `or')
  ECHO_ITEM(`Forward by Subject', `os')
  ECHO_ITEM(`Forward by Thread', `ot')
  ECHO_ITEM(`Forward by Size', `oz')
  ECHO_ITEM(`Forward by Mailbox Order', `ou')
end
begin menu
  title Reverse
  ECHO_ITEM(`Reverse by Message Date', `Od')
  ECHO_ITEM(`Reverse by Sender', `Of')
  ECHO_ITEM(`Reverse by Date Received', `Or')
  ECHO_ITEM(`Reverse by Subject', `Os')
  ECHO_ITEM(`Reverse by Thread', `Ot')
  ECHO_ITEM(`Reverse by Size', `Oz')
  ECHO_ITEM(`Reverse by Mailbox Order', `Ou')
end
begin menu
  title Sort
  SUBMENU_ITEM(`Forward')
  SUBMENU_ITEM(`Reverse')
end

begin menu
  title Mailbox
  SUBMENU_ITEM(`Change Folder')
  SUBMENU_ITEM(`Sort')
  -
  ECHO_ITEM(`Resync', `$')
end
')
dnl### END mutt menus

dnl### BEGIN vim menus
ifdef(`VIM', `
begin menu
  title Method
  ECHO_ITEM(`Manual', `\e:set fdm=manual\r')
  ECHO_ITEM(`Indent', `\e:set fdm=indent\r')
  ECHO_ITEM(`Expression', `\e:set fdm=expr\r')
  ECHO_ITEM(`Syntax', `\e:set fdm=syntax\r')
  ECHO_ITEM(`Diff', `\e:set fdm=diff\r')
  ECHO_ITEM(`Marker', `\e:set fdm=marker\r')
end
begin menu
  title F_Width
  ECHO_ITEM(`0', `\e:set fdc=0\r')
  ECHO_ITEM(`1', `\e:set fdc=1\r')
  ECHO_ITEM(`2', `\e:set fdc=2\r')
  ECHO_ITEM(`3', `\e:set fdc=3\r')
  ECHO_ITEM(`4', `\e:set fdc=4\r')
  ECHO_ITEM(`5', `\e:set fdc=5\r')
  ECHO_ITEM(`6', `\e:set fdc=6\r')
  ECHO_ITEM(`7', `\e:set fdc=7\r')
  ECHO_ITEM(`8', `\e:set fdc=8\r')
end
begin menu
  title Folding
  ECHO_ITEM(`Enable/Disable Folding', `\ezi')
  ECHO_ITEM(`View Curosr Line', `\ezv')
  ECHO_ITEM(`View Cursor Line only', `\ezMzx')
  ECHO_ITEM(`Close more folds', `\ezm')
  ECHO_ITEM(`Close all folds', `\ezM')
  ECHO_ITEM(`Open more folds', `\ezr')
  ECHO_ITEM(`Open all folds', `\ezR')
  -
  SUBMENU_ITEM(`Method', `Method')
  ECHO_ITEM(`Create Fold', `\ezf')
  ECHO_ITEM(`Delete Fold', `\ezd')
  ECHO_ITEM(`Delete All Folds', `\ezD')
  -
  SUBMENU_ITEM(`Fold Column Width', `F_Width')
end
begin menu
  title Shiftwidth
  ECHO_ITEM(`2', `\e:set shiftwidth=2\r')
  ECHO_ITEM(`3', `\e:set shiftwidth=3\r')
  ECHO_ITEM(`4', `\e:set shiftwidth=4\r')
  ECHO_ITEM(`5', `\e:set shiftwidth=5\r')
  ECHO_ITEM(`6', `\e:set shiftwidth=6\r')
  ECHO_ITEM(`7', `\e:set shiftwidth=7\r')
  ECHO_ITEM(`8', `\e:set shiftwidth=8\r')
end
begin menu
  title Settings
  ECHO_ITEM(`Toggle Line Numbering', `\e:set nu!\r')
  ECHO_ITEM(`Toggle Line Wrap', `\e:set wrap!\r')
  ECHO_ITEM(`Toggle Search Pattern Hilight', `\e:set hls!\r')
  ECHO_ITEM(`Toggle Expand Tab', `\e:set et!\r')
  ECHO_ITEM(`Toggle auto-indent', `\e:set ai!\r')
  -
  STRING_ITEM(`Toggle Toolbar', `\e]6;14\a')
  STRING_ITEM(`Toggle Scrollbar Left/Right', `\e]6;11\a')
  STRING_ITEM(`Toggle Scrollbar Visible', `\e[?30t')
  -
  SUBMENU_ITEM(`Shiftwidth', `Shiftwidth')
  ECHO_ITEM(`Text Width', `\e:set textwidth=')
end
begin menu
  title File
  ECHO_ITEM(`Open...', `[:e]', `\e:e')
  ECHO_ITEM(`Split-Open...', `[:sp]', `\e:sp')
  ECHO_ITEM(`New', `[:enew]', `\e:enew\r')
  -
  ECHO_ITEM(`Save', `[:w]', `\e:w\r')
  ECHO_ITEM(`Save As...', `[:saveas]', `\e:saveas')
  -
  ECHO_ITEM(`Show Diff with...', `[:diffsplit]', `\e:diffsplit')
  ECHO_ITEM(`Show Patched by...', `(broken)', `o broken in gvim too')
  -
  ECHO_ITEM(`Print', `[:print]', `\e:print')
  -
  ECHO_ITEM(`Save-Exit', `[:wq]', `\e:wq\r')
  ECHO_ITEM(`Exit', `[:q]', `\e:q\r')
end
begin menu
  title Edit
  ECHO_ITEM(`Undo', `[:u]', `\e:u\r')
  ECHO_ITEM(`Redo', `[:red]', `\e:red\r')
  ECHO_ITEM(`Repeat', `[.]', `\e.')
  -
  ECHO_ITEM(`Cut', `[+x]', `+x')
  ECHO_ITEM(`Copy', `[+y]', `+y')
  ECHO_ITEM(`Paste', `[^Y]', `C-Y')
  ECHO_ITEM(`Put Before', `\e[p\r')
  ECHO_ITEM(`Put After', `\e]p\r')
  ECHO_ITEM(`Select All', `[ggVG]', `ggVG')
  -
  ECHO_ITEM(`Find', `[/]', `\e/')
  ECHO_ITEM(`Find and Replace', `[:%s/]', `\e:\%s/')
  -
  ECHO_ITEM(`Settings Window', `[:options]', `\e:options\r')
  SUBMENU_ITEM(`Settings', `Settings')
end
begin menu
  title Tools
  ECHO_ITEM(`Jump to this tag[x]', `\eg^]')
  ECHO_ITEM(`Jump back', `i\e^T')  
  ECHO_ITEM(`Build Tags File', `\e:!ctags -R .\r')
  -
  SUBMENU_ITEM(`Folding', `Folding')
  -
  ECHO_ITEM(`Make', `[:make]', `\e:make\r')
  ECHO_ITEM(`List Errors', `[:cl]', `\e:cl\r')
  ECHO_ITEM(`List Messages', `[:cl!]', `\ecl!\r')
  ECHO_ITEM(`Next Error', `[:cn]', `\e:cn\r')
  ECHO_ITEM(`Previous Error', `[:cp]', `\e:cp\r')
  ECHO_ITEM(`Older List', `[:cold]', `\e:cold\r')
  ECHO_ITEM(`Newer List', `[:cnew]', `\e:cnew\r')
  ECHO_ITEM(`Error Window', `[:cwin]', `\e:cwin\r')
  -
  ECHO_ITEM(`Convert to HEX', `\e:let b:mod = &mod\r:silent \%!xxd\r:set ft=xxd\r:let &mod = b:mod\r')
  ECHO_ITEM(`Convert Back', `\e:let b:mod = &mod\e:\%!xxd -r\r:doautocmd filetypedetect BufReadPost\r:let &mod = b:mod\r')
end
begin menu
  title Syntax
  ECHO_ITEM(`Set syntax only', `\e:let s:syntax_menu_synonly=1\r')
  ECHO_ITEM(`Off', `[:syn off]', `\e:syn off\r')
  ECHO_ITEM(`Manual', `[:syn manual]', `\e:syn manual\r')
  ECHO_ITEM(`Automatic', `[:syn on]', `\e:syn on\r')
  ECHO_ITEM(`on (this file)', `\e:set syn=ON\r')
  ECHO_ITEM(`off (this file)', `\esyn clear\r')
  -
  ECHO_ITEM(`Color test', `\e:sp \$VIMRUNTIME/syntax/colortest.vim\r:source \%\r8Gz\r')
  ECHO_ITEM(`Hilight test', `\e:so \$VIMRUNTIME/syntax/hitest.vim\r')
  ECHO_ITEM(`Convert to HTML', `\e:so \$VIMRUNTIME/syntax/2html.vim\r')
end
begin menu
  title Move_To
  ECHO_ITEM(`Top', `\e^WK')
  ECHO_ITEM(`Bottom', `\e^WK')
  ECHO_ITEM(`Left', `\e^WH')
  ECHO_ITEM(`Right', `\e^WL')
end
begin menu
  title Window
  ECHO_ITEM(`New', `\e^Wn')
  ECHO_ITEM(`Split', `\e^Ws')
  ECHO_ITEM(`Split to \#', `\e^W^^')
  ECHO_ITEM(`Split Vertically', `\e^wv')
  ECHO_ITEM(`Split File Explorer', `\e:20vsp .\r')
  -
  ECHO_ITEM(`Close', `\e^Wc')
  ECHO_ITEM(`Close Other(s)', `^Wo')
  -
  SUBMENU_ITEM(`Move To', `Move_To')
  ECHO_ITEM(`Rotate Up', `\e^WR')
  ECHO_ITEM(`Rotate Down', `\e^Wr')
  -
  ECHO_ITEM(`Equal Size', `\e^W=')
  ECHO_ITEM(`Max Height', `\e^W_')
  ECHO_ITEM(`Min Height', `\e^W1_')
  ECHO_ITEM(`Max Width', `\e^W\|')
  ECHO_ITEM(`Min Width', `\e^W1\|')
end
begin menu
  title Help
  ECHO_ITEM(`Overview', `\e:h\r')
  ECHO_ITEM(`How-to links', `\e:h how-to\r')
  ECHO_ITEM(`Credits', `\e:h credits\r')
  ECHO_ITEM(`Copying', `\e:h uganda\r')
  ECHO_ITEM(`Find...', `\e:helpfind')
  -
  ECHO_ITEM(`Version', `\e:version\r')
  ECHO_ITEM(`About', `\e:intro\r')
end
')
dnl### END vim menus

dnl### BEGIN Emacs menus
ifdef(`EMACS', `
begin menu
  title Buffers
  ECHO_ITEM(`List all', `C-x C-b', `C-xC-b')
  ECHO_ITEM(`Change to other', `C-x o', `C-xo')
  ECHO_ITEM(`Change to...', `C-x b', `C-xb')
end
begin menu
  title Files
  ECHO_ITEM(`Open File...', `C-x C-f', `C-xC-f')
  ECHO_ITEM(`Open Directory...', `C-x d', `C-xd')
  ECHO_ITEM(`Save Buffer', `C-x C-s', `C-xC-s')
  ECHO_ITEM(`Save Buffer As...', `C-x C-w', `C-xC-w')
  ECHO_ITEM(`Revert Buffer', `M-x revert-buffer\r', `M-xrevert-buffer\r')
  ECHO_ITEM(`Insert File...', `C-x i', `C-xi')
  ECHO_ITEM(`Kill Current Buffer', `M-x kill-this-buffer\r', `M-xkill-this-buffer\r')
  -
  ECHO_ITEM(`Make New Frame', `C-x 52', `C-x52')
  ECHO_ITEM(`Open New Display...', `M-x make-frame-on-display', `M-xmake-frame-on-display')
  ECHO_ITEM(`Delete Frame', `C-x 50', `C-x50')
  ECHO_ITEM(`Split Window', `C-x 2', `C-x2')
  ECHO_ITEM(`One Window', `C-x 1', `C-x1')
  -
  ECHO_ITEM(`Exit Emacs', `C-x C-c', `C-xC-c')
end
begin menu
  title Edit
  ECHO_ITEM(`Undo', `C-_', `C-_')
  -
  ECHO_ITEM(`Set Mark', `C-\40', `C-\40')
  ECHO_ITEM(`Cut', `C-w', `C-w')
  ECHO_ITEM(`Copy', `M-w', `M-w')
  ECHO_ITEM(`Paste', `C-y', `C-y')
  ECHO_ITEM(`Clear', `M-x delete-region', `M-xdelete-region')
  -
  ECHO_ITEM(`Cut Rectangle', `M-x kill-rectangle', `M-xkill-rectangle')
  ECHO_ITEM(`Clear Rectangle', `M-x clear-rectangle', `M-xclear-rectangle')
end
begin menu
  title Search
  ECHO_ITEM(`Search...', `C-s', `C-s')
  ECHO_ITEM(`Regexp Search...', `M-x isearch-forward-regexp', `M-xisearch-forward-regexp')
  ECHO_ITEM(`Search Backward...', `C-r', `C-r')
  ECHO_ITEM(`Regexp Search Backward...', `M-x isearch-backward-regexp', `M-xisearch-backward-regexp')
  -
  ECHO_ITEM(`Repeat Search', `M-x isearch-repeat-forward', `M-xisearch-repeat-forward')
  ECHO_ITEM(`Repeat Regexp Search', `M-x isearch-repeat-forward', `M-xisearch-repeat-forward')
  ECHO_ITEM(`Repeat Search Backward', `M-x isearch-repeat-backward', `M-xisearch-repeat-backward')
  ECHO_ITEM(`Repeat Regexp Search Backward', `M-x isearch-repeat-backward', `M-xisearch-repeat-backward')
  -
  ECHO_ITEM(`Replace String...', `M-x replace-string', `M-xreplace-string')
  ECHO_ITEM(`Replace Regexp...', `M-x replace-regexp', `M-xreplace-regexp')
  ECHO_ITEM(`Query Replace String...', `M-x query-replace', `M-xquery-replace')
  ECHO_ITEM(`Query Replace Regexp...', `M-x query-replace-regexp', `M-xquery-replace-regexp')
  -
  ECHO_ITEM(`Goto Line...', `M-x goto-line', `M-xgoto-line')
end
begin menu
  title Help
  ECHO_ITEM(`Customize', `M-x customize', `M-xcustomize')
  ECHO_ITEM(`Emacs News', `M-x view-emacs-news', `M-xview-emacs-news')
  ECHO_ITEM(`Emacs FAQ', `M-x view-emacs-FAQ', `M-xview-emacs-FAQ')
  ECHO_ITEM(`Info Browser', `M-x info', `M-xinfo')
  -
  ECHO_ITEM(`Describe Mode', `M-x describe-mode', `M-xdescribe-mode')
  ECHO_ITEM(`Command Apropos...', `M-x command-apropos', `M-xcommand-apropos')
  ECHO_ITEM(`List Keybindings', `M-x describe-bindings', `M-xdescribe-bindings')
  ECHO_ITEM(`Describe Key...', `M-x describe-key', `M-xdescribe-key')
  ECHO_ITEM(`Describe Function...', `M-x describe-function', `M-xdescribe-function')
  ECHO_ITEM(`Describe Variable...', `M-x describe-variable', `M-xdescribe-variable')
  -
  ECHO_ITEM(`Man...', `M-x man', `M-xman')
  ECHO_ITEM(`Emacs Tutorial', `M-x help-with-tutorial', `M-xhelp-with-tutorial')
  ECHO_ITEM(`Find Lisp Packages...', `M-x finder-by-keyword', `M-xfinder-by-keyword')
  ECHO_ITEM(`Send Bug Report...', `M-x report-emacs-bug', `M-xreport-emacs-bug')
  -
  ECHO_ITEM(`Show Version', `M-x version', `M-xversion')
end
')
dnl### END Emacs menus

dnl### BEGIN xterm menus
ifdef(`XTERM', `
begin menu
  title "Eterm Options"
  SUBMENU_ITEM(`Scrollbar')
  SUBMENU_ITEM(`Move to Desktop')
  SUBMENU_ITEM(`Pixmap')
  SUBMENU_ITEM(`Brightness')
  SUBMENU_ITEM(`Contrast')
  SUBMENU_ITEM(`Gamma')
  -
  STRING_ITEM(`Steal Focus', `Ctrl-Button1', `\e]5;\a')
  SCRIPT_ITEM(`New Eterm Window', `spawn(Eterm)')
  SCRIPT_ITEM(`Search...', `search()')
  SCRIPT_ITEM(`Run...', `exec_dialog(Eterm -e )')
  -
  STRING_ITEM(`Version', `\e[8n')
  STRING_ITEM(`Status', `\e[9n')
  separator
  SCRIPT_ITEM(`Save User Settings...', `save')
  SCRIPT_ITEM(`Save Theme Settings...', `save(theme)')
  SCRIPT_ITEM(`Exit', `exit')
end
m4exit(0)
')
dnl### END xterm menus

begin menu
  title ifdef(`VIM', `Vim', `Eterm')
  SUBMENU_ITEM(Font)
  SUBMENU_ITEM(Background)
  SUBMENU_ITEM(Terminal)
  -
ifdef(`CHOOSER', `
  SUBMENU_ITEM(`ssh')
  SUBMENU_ITEM(`rlogin')
  SUBMENU_ITEM(`telnet')
  SUBMENU_ITEM(`ftp')
  -
')
ifdef(`MUTT', `
  SUBMENU_ITEM(`Messages')
  SUBMENU_ITEM(`Mailbox')
  -
')
ifdef(`VIM', `
  SUBMENU_ITEM(`File', `File')
  SUBMENU_ITEM(`Edit', `Edit')
  SUBMENU_ITEM(`Tools', `Tools')
  SUBMENU_ITEM(`Syntax', `Syntax')
  SUBMENU_ITEM(`Window', `Window')
  SUBMENU_ITEM(`Help', `Help')
  -
')
  SCRIPT_ITEM(`New Eterm Window', `spawn(Eterm)')
  SCRIPT_ITEM(`Search...', `search()')
  SCRIPT_ITEM(`Run...', `exec_dialog(Eterm -e )')
  -
  STRING_ITEM(`Version', `\e[8n')
  STRING_ITEM(`Status', `\e[9n')
  separator
  SCRIPT_ITEM(`Save User Settings...', `save')
  SCRIPT_ITEM(`Save Theme Settings...', `save(theme)')
  SCRIPT_ITEM(`Exit', `exit')
end

begin menu
  title "Eterm Operations"
  SCRIPT_ITEM(`New Eterm Window', `spawn(Eterm)')
  SCRIPT_ITEM(`Search...', `search()')
  SCRIPT_ITEM(`Run...', `exec_dialog(Eterm -e )')
  -
  STRING_ITEM(`Version', `\e[8n')
  STRING_ITEM(`Status', `\e[9n')
  separator
  SCRIPT_ITEM(`Save User Settings...', `save')
  SCRIPT_ITEM(`Save Theme Settings...', `save(theme)')
  SCRIPT_ITEM(`Exit', `exit')
end
