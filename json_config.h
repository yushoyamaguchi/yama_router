typedef struct	{
	char	*Device1;
	char	*Device2;
	int     DebugOut;
	char	*NextRouter;
}PARAM;

typedef struct	{
	char	*Device[20];
	int     DebugOut;
	char	*NextRouter;
}PARAM_new;


void json_read(PARAM_new *pa,json_t *json_object,json_error_t *jerror);