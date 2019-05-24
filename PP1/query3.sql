SELECT COUNT(*)
FROM
(
    SELECT COUNT(DISTINCT Type) as c
    FROM Categorize
    GROUP BY ItemID
) countTable
Where countTable.c = 4