# 자주 사용하는 명령어 모음

이 문서는 실습 중 자주 사용하는 명령어들을 복사-붙여넣기로 쉽게 사용할 수 있도록 정리한 것입니다.

## Git Flow 명령어

수업 중 기능 단위 개발을 위해 Git Flow를 사용합니다. 터미널(Terminal)에서 아래 명령어를 사용하세요.

### 초기화 (최초 1회)

```bash
git flow init -d
```

### 작업 시작

```bash
git flow feature start 실습이름
```

### 작업 완료

```bash
git flow feature finish 실습이름
```

## Make 명령어

### 프로젝트 빌드

```bash
make
```

또는

```bash
make all
```

### 테스트 실행 (Unity 테스트가 있는 프로젝트)

```bash
make test
```

### 빌드 산출물 삭제

```bash
make clean
```

## Git 기본 명령어

### 저장소 클론

```bash
git clone <저장소_URL>
cd c_programming_base
```

### Git 버전 확인

```bash
git --version
```
