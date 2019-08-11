create or replace function create_session_track (
    in in_user_name        varchar(32),
    in in_album_release_id int,
    in in_disc_num         int,
    in in_track_num        int
)
returns int
language plpgsql
as $$
    declare next_num int;
begin
    select coalesce(max("num"), 0) + 1
      from "session_track"
     where "user_name" = in_user_name
      into next_num;

    insert into "session_track" ("user_name", "album_release_id", "disc_num", "track_num", "num")
         values (in_user_name, in_album_release_id, in_disc_num, in_track_num, next_num);

    return next_num;
end;
$$;
