create or replace function get_album_formats (
    in in_album_release_id int
)
returns table (
    "code"     varchar,
    "quality"  int,
    "lossless" int
)
language plpgsql
as $$
begin
      return query
      select "audio_format"."code",
             "audio_format"."quality",
             "audio_format"."lossless"
        from "album_release_format"
        join "audio_format"
          on "audio_format"."code" = "album_release_format"."audio_format_code"
       where "album_release_format"."album_release_id" = in_album_release_id;
end;
$$;
