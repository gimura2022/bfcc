NAME = bfcc

POSTFIX ?= usr/local
DESTDIR ?= /

RM ?= rm -rf

.PHONY: all
all: $(NAME) lib$(NAME).a lib$(NAME).so README

lib$(NAME).a: lib$(NAME).o
	ar rcs lib$(NAME).a lib$(NAME).o

lib$(NAME).so: lib$(NAME).c
	gcc -fpic -shared -o lib$(NAME).so lib$(NAME).c

.PHONY: runtime
runtime: lib$(NAME).a lib$(NAME).so
	install -d $(DESTDIR)$(POSTFIX)/lib/
	install -m 775 lib$(NAME).so $(DESTDIR)$(POSTFIX)/lib/
	install -m 644 lib$(NAME).a $(DESTDIR)$(POSTFIX)/lib/

.PHONY: clean
clean:
	$(RM) $(NAME)
	$(RM) lib$(NAME).so
	$(RM) lib$(NAME).a
	$(RM) *.o
	$(RM) README

.PHONY: install
install: $(NAME) $(NAME).1 runtime
	install -d $(DESTDIR)$(POSTFIX)/bin/
	install -m 775 $(NAME) $(DESTDIR)$(POSTFIX)/bin/
	install -d $(DESTDIR)$(POSTFIX)/share/man/man1/
	install -m 644 $(NAME).1 $(DESTDIR)$(POSTFIX)/share/man/man1/

.PHONY: uninstall
uninstall:
	$(RM) $(DESTDIR)$(POSTFIX)/bin/$(NAME)
	$(RM) $(DESTDIR)$(POSTFIX)/lib/lib$(NAME).a
	$(RM) $(DESTDIR)$(POSTFIX)/lib/lib$(NAME).so
	$(RM) $(DESTDIR)$(POSTFIX)/share/man/man1/$(NAME).1

README: $(NAME).1
	mandoc -mdoc -T ascii $(NAME).1 | col -b > README
