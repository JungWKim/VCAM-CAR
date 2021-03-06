#include "serial.h" 

int fd; // 시리얼포트 파일핸들
char control;
int rdcnt; 

void write_port()
{
    control = -70;
    //sprintf(message, "%d", control);
    
    sleep(3);//시리얼 통신 초기화시 아두이노가 리셋되는 걸 방지

    while(1)
    {
        if((rdcnt = write( fd, &control, sizeof(control))) < 0)
        {
            perror("[failed] write left_or_right");
            exit(1);
        }
        printf("send control data : %d\n", control);
    }
}

int main(int argc, char **argv) {

    int baud; // 전송속도
    char dev_name[128]; // 시리얼포트 노드파일 이름
    char cc; // 데이타 버퍼
    int i = 0;

    if ( argc != 3 )
    {
        printf( " serial [device] [baud]\n" \
        " device : /dev/ttyUSB0 ...\n" \
        " baud : 57600\n" );
        return -1;
    }

    printf( " Serial test start... (%s)\n", __DATE__ ); 

    // 인자를 얻어온다.
    strcpy( dev_name, argv[1] ); // 노드파일 이름
    baud = strtoul( argv[2], NULL, 10 ); // 전송속도

    // 시리얼 포트를 연다
    // 시리얼포트를 1초동안 대기하거나 1바이트 이상의 데이타가 들어오면
    // 깨어나도록 설정한다.
    fd = open_serial( dev_name, baud, 10, 1);
    if ( fd < 0 ) {
        perror("[failed] open serial");
        return -2; 
    }
    printf("open serial socket\n");

    write_port();

    // 시리얼 포트를 닫는다.
    close_serial( fd );
    printf( " Serial test end\n" ); 
    return 0;
}
