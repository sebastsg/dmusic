create or replace function get_session_tracks (
    in in_user_name varchar(32)
)
returns table (
    "album_release_id" int,
    "disc_num"         int,
    "track_num"        int,
    "name"             varchar,
    "seconds"          int
)
language plpgsql
as $$
begin
      return query
      select "session_track"."album_release_id",
             "session_track"."disc_num",
             "session_track"."track_num",
             "track"."name",
             "track"."seconds"
        from "session_track"
        join "track"
          on "track"."album_release_id" = "session_track"."album_release_id"
         and "track"."disc_num" = "session_track"."disc_num"
         and "track"."num" = "session_track"."track_num"
       where "session_track"."user_name" = in_user_name
    order by "session_track"."num";
end;
$$;
