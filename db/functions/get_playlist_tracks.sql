create or replace function get_playlist_tracks (
    in in_playlist_id int
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
      select "playlist_track"."album_release_id",
             "playlist_track"."disc_num",
             "playlist_track"."track_num",
             "track"."name",
             "track"."seconds"
        from "playlist_track"
        join "track"
          on "track"."album_release_id" = "playlist_track"."album_release_id"
         and "track"."disc_num" = "playlist_track"."disc_num"
         and "track"."num" = "playlist_track"."track_num"
       where "playlist_track"."playlist_id" = in_playlist_id
    order by "playlist_track"."num";
end;
$$;
