-- q1_sample
SELECT
	DISTINCT(category)
FROM
	crew
ORDER BY
	category
LIMIT
	10;

-- q2_not_the_same_title
select
	premiered,
	primary_title || " (" || original_title || ")"
from
	titles
where
	primary_title <> original_title
	and type == 'movie'
	and genres like '%Action%'
order by
	premiered desc,
	primary_title asc
limit
	10;

-- q3_longest_running_tv
SELECT
	primary_title,
	CASE
		WHEN ended IS NOT NULL THEN ended - premiered
		ELSE 2023 - premiered
	END AS runtime
FROM
	titles
WHERE
	premiered NOT NULL
	AND type = 'tvSeries'
ORDER BY
	runtime DESC,
	primary_title ASC
LIMIT
	20;

-- q4_directors_in_each_decade
SELECT
	CAST(born / 10 * 10 AS TEXT) || 's' AS decade,
	COUNT(DISTINCT(people.person_id)) AS num_directors
FROM
	people
	INNER JOIN crew ON people.person_id = crew.person_id
WHERE
	born IS NOT NULL
	AND category == 'director'
	AND born >= 1900
GROUP BY
	decade
ORDER BY
	decade;

-- q5_german_type_ratings
SELECT
	t.type,
	ROUND(AVG(r.rating), 2) AS avg_rating,
	MIN(r.rating),
	MAX(r.rating)
FROM
	akas as a
	INNER JOIN ratings as r ON r.title_id = a.title_id
	INNER JOIN titles as t ON t.title_id = a.title_id
WHERE
	a.language = 'de'
	AND (
		a.types = 'imdbDisplay'
		OR a.types = 'original'
	)
GROUP BY
	t.type
ORDER BY
	avg_rating;

-- q6_who_played_a_batman
WITH batman_actors AS (
	SELECT
		DISTINCT(people.person_id) AS id,
		name
	FROM
		people
		INNER JOIN crew ON people.person_id = crew.person_id
	WHERE
		characters LIKE '%"Batman"%'
		AND category == 'actor'
)
SELECT
	name,
	ROUND(AVG(rating), 2) as avg_rating
FROM
	ratings
	INNER JOIN crew ON ratings.title_id == crew.title_id
	INNER JOIN batman_actors ON crew.person_id == batman_actors.id
GROUP BY
	batman_actors.id
ORDER BY
	avg_rating DESC
LIMIT
	10;

-- q7_born_with_prestige
SELECT
	COUNT(DISTINCT people.person_id)
FROM
	people
	INNER JOIN crew ON people.person_id == crew.person_id
WHERE
	born IN (
		SELECT
			premiered
		FROM
			titles
		WHERE
			primary_title == 'The Prestige'
	)
	AND (
		category == 'actor'
		OR category == 'actress'
	)
ORDER BY
	name;

-- q8_directing_rose
WITH rose_titles AS (
	SELECT
		DISTINCT(c.title_id)
	FROM
		people AS p
		INNER JOIN crew AS c ON c.person_id = p.person_id
	WHERE
		category = 'actress'
		AND name LIKE 'Rose%'
),
rose_directors AS (
	SELECT
		DISTINCT(p.person_id)
	FROM
		people AS p
		INNER JOIN crew AS c ON c.person_id = p.person_id
	WHERE
		c.category = 'director'
		AND c.title_id IN rose_titles
)
SELECT
	name
FROM
	people AS p
	INNER JOIN rose_directors AS rd ON rd.person_id = p.person_id
ORDER BY
	name ASC;

-- q9_ode_to_death
WITH longest_per_person AS (
	SELECT
		category,
		name,
		died,
		primary_title,
		runtime_minutes,
		DENSE_RANK() OVER (
			PARTITION BY c.category
			ORDER BY
				p.died ASC,
				p.name ASC
		) AS rank_num,
		DENSE_RANK() OVER (
			PARTITION BY c.category,
			c.person_id
			ORDER BY
				runtime_minutes DESC,
				t.title_id ASC
		) AS rank_num_runtime
	FROM
		crew AS c
		INNER JOIN people as p ON p.person_id = c.person_id
		INNER JOIN titles as t ON c.title_id = t.title_id
	WHERE
		p.died NOT NULL
		AND runtime_minutes NOT NULL
),
top_titles AS (
	SELECT
		category,
		name,
		died,
		primary_title,
		runtime_minutes,
		rank_num
	FROM
		longest_per_person as lpp
	WHERE
		rank_num <= 5
		AND rank_num_runtime = 1
)
SELECT
	*
FROM
	top_titles
ORDER BY
	category ASC,
	rank_num ASC;

-- q10_all_played_by_leo
WITH json_table(json_data) AS (
	SELECT
		c.characters as json_data
	FROM
		people AS p,
		crew AS c
	WHERE
		p.name = 'Leonardo DiCaprio'
		AND p.born = 1974
		AND p.person_id = c.person_id
	ORDER BY
		c.characters
),
characters(character) AS (
	SELECT
		DISTINCT value as character
	FROM
		json_table,
		json_each(json_table.json_data)
	WHERE
		character <> ''
		AND character NOT LIKE '%SELF%'
	ORDER BY
		character
)
SELECT
	GROUP_CONCAT(character)
FROM
	characters;