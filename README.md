# keyCounter
Gets statistical data from keys pressed over time and summarize it

I made this program in 2010, just for fun. I know some things don't
work right, But I'm not maintaining this code now.

If you want to make something better, please fork this program so I
will know you're working on it, and it would be nice to see your work.

You run this program and has statistical data, you can send how many
keys you pressed during a week:

$ ./keyCounter analyze hourly

or which are the most pressed keys

$ ./keyCounter analyze keycount | sort -t';' -n -k2

it would be interesting, and I would include these stats here, or make
by country stats, by main programming language, or even more, when I have
enough data.
