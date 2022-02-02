typedef struct	{
	char	*Device1;
	char	*Device2;
	int     DebugOut;
	char	*NextRouter;
}PARAM;


void json_read(PARAM *pa,json_t *json_object,json_error_t *jerror);