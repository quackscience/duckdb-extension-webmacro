# DuckDB WebMacro

This extension allows loading DuckDB Macros (both scalar and table) from URLs, gists, pasties, etc.

### Installation

```sql
INSTALL webmacro FROM community;
LOAD webmacro;
```

### Usage
Create a DuckDB SQL Macro and save it somewhere. Here's an [example](https://gist.githubusercontent.com/lmangani/518215a68e674ac662537d518799b893)

Load your remote macro onto your system using a URL: 

```sql
SELECT load_macro_from_url('https://gist.githubusercontent.com/lmangani/518215a68e674ac662537d518799b893/raw/5f305480fdd7468f4ecda3686011bab8e8e711bf/bsky.sql') as res;
┌─────────────────────────────────────────┐
│                   res                   │
│                 varchar                 │
├─────────────────────────────────────────┤
│ Successfully loaded macro: search_posts │
└─────────────────────────────────────────┘
```

Use your new macro and have fun: 

```sql
D SELECT * FROM search_posts('qxip.bsky.social', text := 'quack');
┌──────────────────┬──────────────┬──────────────────────┬───┬─────────┬─────────┬───────┬────────┐
│  author_handle   │ display_name │      post_text       │ … │ replies │ reposts │ likes │ quotes │
│     varchar      │   varchar    │       varchar        │   │  int64  │  int64  │ int64 │ int64  │
├──────────────────┼──────────────┼──────────────────────┼───┼─────────┼─────────┼───────┼────────┤
│ qxip.bsky.social │ qxip         │ This is super cool…  │ … │       1 │       0 │     1 │      0 │
│ qxip.bsky.social │ qxip         │ github.com/quacksc…  │ … │       0 │       1 │     2 │      0 │
│ qxip.bsky.social │ qxip         │ #DuckDB works grea…  │ … │       2 │       3 │    24 │      0 │
│ qxip.bsky.social │ qxip         │ github.com/quacksc…  │ … │       1 │       0 │     0 │      0 │
│ qxip.bsky.social │ qxip         │ The latest #Quackp…  │ … │       0 │       0 │     2 │      0 │
│ qxip.bsky.social │ qxip         │ The #DuckDB Ecosys…  │ … │       0 │       0 │     5 │      0 │
│ qxip.bsky.social │ qxip         │ Ladies and Gents, …  │ … │       1 │       0 │     4 │      0 │
├──────────────────┴──────────────┴──────────────────────┴───┴─────────┴─────────┴───────┴────────┤
│ 7 rows                                                                      9 columns (7 shown) │
└─────────────────────────────────────────────────────────────────────────────────────────────────┘
```
