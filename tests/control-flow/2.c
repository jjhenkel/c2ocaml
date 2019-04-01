void example() {
    void *A,*B;
    if (!strcasecmp()) {
        addReplyHelp();
    } else if (!strcasecmp()) {
        A = dictGetIterator();
        addReplyDeferredLen();
        while ((B = dictNext(A)) != 0) {
            dictGetKey(B);
            if (stringmatchlen()) {
                addReplyBulk();
            }
        }
        dictReleaseIterator(A);
    } else if (!strcasecmp()) {
        addReplyLongLong(listLength());
    } else {
        addReplySubcommandSyntaxError();
    }
}
